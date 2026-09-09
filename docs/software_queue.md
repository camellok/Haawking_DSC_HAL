# 泛型软件队列

`HAL_QUEUE`是目标无关、固定容量、先进先出的环形队列。它不依赖DriverLib，不绑定CAN、SCI、ADC等任何外设，也不使用动态内存。

## 存储与所有权

调用者负责静态分配队列对象和连续存储区。`HAL_QUEUE_init()`只保存不透明的存储区地址、元素大小和容量，不复制或接管存储区。`elementSize`应由`sizeof`得到，其单位是目标C实现的存储单位，不假设一个单位固定为8 bit。

```c
typedef struct
{
    uint16_t identifier;
    uint8_t length;
    uint8_t data[8];
} ApplicationFrame_t;

static HAL_QUEUE_Obj applicationQueue;
static ApplicationFrame_t applicationQueueStorage[16];

HAL_Status_t status = HAL_QUEUE_init(&applicationQueue,
                                    applicationQueueStorage,
                                    sizeof(applicationQueueStorage[0]),
                                    16U);
```

存储区在队列使用期间必须持续有效。队列对象初始化后不得按值复制，因为副本会共享同一存储区但持有独立索引。

## 操作语义

| 接口 | 行为 |
|---|---|
| `HAL_QUEUE_init()` | 验证参数并把队列重置为空；失败时不修改对象 |
| `HAL_QUEUE_clear()` | 丢弃全部逻辑元素，不擦除存储区字节 |
| `HAL_QUEUE_push()` | 复制一个元素到队尾；队满返回`HAL_STATUS_FULL`且不覆盖旧数据 |
| `HAL_QUEUE_pop()` | 复制并移除队首；队空返回`HAL_STATUS_EMPTY`且不修改输出 |
| `HAL_QUEUE_peek()` | 复制队首但不移除 |
| `HAL_QUEUE_getCount()` | 返回已有元素数量 |
| `HAL_QUEUE_getFreeCount()` | 返回剩余容量 |
| `HAL_QUEUE_isEmpty()` | 返回队列是否为空 |
| `HAL_QUEUE_isFull()` | 返回队列是否已满 |

所有元素都按初始化时的`elementSize`完整复制。调用者必须保证入队源对象和出队目标对象至少具有该大小。

## 并发边界

队列不在内部禁用中断，也不使用锁或C11原子操作。一次入队或出队会更新存储内容、索引和计数，因此同一对象默认只能串行访问。

当ISR与主循环、两个ISR或多任务共享同一队列时，宿主必须提供临界区或调度器同步。临界区策略依赖目标架构、中断优先级和实时约束，不属于泛型队列实现。

## 构建

把以下文件加入宿主构建：

```text
Include: Haawking_DSC_HAL/include
Source : Haawking_DSC_HAL/source/common/hal_queue.c
```

主机单元测试位于`tests/test_hal_queue.c`，GitHub Actions使用C11和`-Wall -Wextra -Werror`编译并运行。
