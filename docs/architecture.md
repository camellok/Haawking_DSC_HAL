# HAL架构

本仓库采用公共接口与芯片族目标实现分离的结构。公共头文件只表达跨目标稳定的能力和数据所有权；目标源码负责把物理配置转换为DriverLib调用或必要的寄存器操作。队列等不依赖硬件的共用组件放在`source/common`，可由任意目标按需编译。

```text
main / composition root
        |                         application / service
        v                                  |
platform / board binding -----------------+
        |                                  |
        +-------- owns objects --------> HAL API
                                           |
                                  selected target source
                                           |
                                  host-provided DriverLib
```

## 依赖规则

- 公共头文件只依赖C标准头文件和本仓库公共类型。
- `source/common`只实现目标无关的软件能力，不包含DriverLib、板级资源或协议逻辑。
- 目标源码可以包含`driverlib.h`，但不包含产品业务或板级常量。
- reference platform可以包含DriverLib以展示时钟和中断绑定。
- 一个构建只选择一个目标芯片目录。
- HAL对象由platform静态分配；HAL不使用堆。

## 并发边界

目标无关软件组件不自行开关全局中断。泛型队列的单次操作会更新多个索引和计数字段，默认要求调用者串行访问；若ISR和主循环共享同一队列，宿主必须在platform或应用集成层提供与目标实时约束匹配的临界区。

## 兼容性

公共API是版本兼容性的主要边界。reference platform、目标工程文件和验证材料可以随硬件需求演进，但修改仍需在CHANGELOG记录。`0.x`阶段允许经过说明的接口调整；发布`1.0.0`后，破坏兼容性的修改提升主版本号。
