# HAL_CAN-1.0冻结候选评审记录

> 状态：进行中，尚未冻结  
> 候选来源：Haawking_DSC_Training_CANopen P2-L07  
> 目标：HXS320F28002x DCAN

公共接口必要性、实现状态和证据边界见[能力与接口矩阵](hal_can_1_0_capability_matrix.md)。

## 已完成静态审查

- 公共头文件不依赖DriverLib，运行期不使用动态内存；
- 公共头文件可由C++11宿主包含并保持C linkage；
- 普通收发、事件队列和单对象事务均为有界操作；全对象TX取消最多遍历32个消息对象；
- DriverLib的无界消息RAM初始化已替换为目标私有有界事务；
- DriverLib无界消息对象配置已替换为目标私有的单次有界IF1事务；
- IF1用于前台TX事务，IF2用于RX/ISR事务，所有权边界明确；
- `HAL_CAN_setAutoRetransmission()`直接更新硬件；HAL不保留不可查询的配置快照；
- 唯一未实现公共接口`HAL_CAN_softReset()`具有明确的`UNSUPPORTED`和无副作用语义；
- 通用队列升级为Sequence型SPSC，并保留满队列不覆盖旧数据的策略。

## 复杂度结论

| 路径 | 上界 |
|---|---|
| Frame数据复制 | 最多8字节 |
| SPSC push/pop | 一个固定大小元素复制，O(elementSize) |
| 单消息对象IF事务 | 固定寄存器操作加目标私有等待上限 |
| 全部TX取消 | 最多32个对象，每个对象最多三次IF1等待 |
| 诊断采集 | 固定数量DriverLib寄存器读取和状态映射 |

## 静态资源测量

以下数据由HXS320F28002x目标编译器在`-O2`下获得，用于冻结候选的相对比较。目标文件代码尺寸尚未经过最终链接和未引用节裁剪，因此不能等同于应用最终Flash增量。

### 目标ABI类型尺寸

| 类型 | 字节数 |
|---|---:|
| `HAL_CAN_TxCompleteEvent_t` | 2 |
| `HAL_CAN_BitTiming_t` | 10 |
| `HAL_CAN_RemoteRequestConfig_t` | 16 |
| `HAL_CAN_TxConfig_t` | 16 |
| `HAL_CAN_Frame_t` | 20 |
| `HAL_CAN_InterruptConfig_t` | 20 |
| `HAL_CAN_RxConfig_t` | 20 |
| `HAL_CAN_Config_t` | 24 |
| `HAL_CAN_RxEvent_t` | 24 |
| `HAL_QUEUE_Obj` | 32 |
| `HAL_CAN_RemoteResponseConfig_t` | 32 |
| `HAL_CAN_Diagnostics_t` | 56 |
| `HAL_CAN_Obj` | 140 |

队列常驻RAM还包括调用方提供的存储区：RX队列为`24 * capacity`字节，TX完成队列为`2 * capacity`字节。两者不在`HAL_CAN_Obj`的140字节内。冻结审查删除了对象中从未被运行期读取的`HAL_CAN_Config_t`副本，使每个CAN实例减少24字节；配置由platform持有并可放入Flash。

### 独立目标文件代码尺寸

| 目标文件 | `.text` | `.data` | `.bss` |
|---|---:|---:|---:|
| `hal_can.o` | 4222 | 0 | 0 |
| `hal_can_hw.o` | 1642 | 0 | 0 |
| CAN核心合计 | 5864 | 0 | 0 |
| `hal_queue.o` | 842 | 0 | 0 |

### 编译器静态直接栈帧

| 范围 | 直接栈帧 |
|---|---:|
| `HAL_CAN_configureInterrupts()` | 224字节 |
| `HAL_CAN_init()` | 144字节 |
| `HAL_CAN_updateRemoteResponse()` | 64字节 |
| 多数配置、收发和队列push/pop接口 | 48字节 |
| 其余CAN硬件事务与简单接口 | 32字节 |

这些数值来自编译器的逐函数静态报告，只表示单个函数的直接栈帧，不表示ISR嵌套或完整调用链峰值。最终ISR栈峰值必须在训练仓库集成构建后结合调用图或目标测量确认。

### 训练工程完整链接尺寸

训练仓库使用Haawking IDE 2.3.17所带Clang 13.0.1及现有工程等价参数（`-Odefault -fno-inline-functions`、Flash链接配置）生成以下完整ELF/HEX。表中数据是应用完整镜像，不是HAL自身增量：

| Lab | text | data | bss | total |
|---|---:|---:|---:|---:|
| P2-L01 | 14697 | 184 | 12 | 14893 |
| P2-L02 | 14925 | 164 | 36 | 15125 |
| P2-L03 | 16095 | 184 | 48 | 16327 |
| P2-L04 | 15234 | 164 | 56 | 15454 |
| P2-L05 | 16511 | 180 | 60 | 16751 |
| P2-L06 | 20412 | 184 | 284 | 20880 |
| P2-L07 | 24410 | 328 | 36 | 24774 |

### CAN ISR静态调用链栈上界

同一目标编译参数下，训练仓库脚本根据`-fstack-usage`逐函数结果和实际非递归调用链计算：

| 工程与路径 | ISR直接栈帧 | 最深被调函数 | 静态调用链上界 |
|---|---:|---:|---:|
| P2-L06 Line 0 RX | 304 | 96 | 400字节 |
| P2-L06 Line 1 TX完成 | 272 | 64 | 336字节 |
| P2-L07 Line 0诊断 | 304 | 96 | 400字节 |
| P2-L07 Line 1 TX完成 | 288 | 64 | 352字节 |

Line 0与Line 1按当前platform约定不可互相嵌套，因此不把两条路径相加。上述结果覆盖有效句柄下的正常、队列满和IF超时返回路径；不包含宿主额外开放的高优先级中断嵌套、断言失败处理或异常入口。运行期栈水位仍由最终产品宿主负责确认。

## 当前实现风险判断

- 所有公开运行期收发事务均有固定数据复制上限和IF等待上限；没有动态分配或递归。
- `HAL_CAN_init()`只在调用期间读取配置，不复制到运行对象；运行期自动重发切换直接更新硬件，避免为不可查询的配置快照常驻24字节RAM。
- `HAL_CAN_init()`要求实例首次使用前整体零初始化，并只允许在未初始化或停止且CAN中断关闭时调用；硬件初始化超时会清除旧队列与中断标志，将对象留在安全的未初始化状态。
- 初始化后的消息对象配置、polling和ISR路径均不调用含无界BUSY循环的DriverLib接口；直接寄存器访问集中在`hal_can_hw`并统一返回`HAL_STATUS_TIMEOUT`。
- `hal_can_hw`仅覆盖DriverLib无法提供有界语义的DCAN操作，不作为其他外设HAL的强制分层模板。
- SPSC队列要求严格的一生产者、一消费者和单核可见性；初始化、清空和重新绑定必须在双方停止时执行。
- IF1要求单一串行前台所有者，Line 0与Line 1 ISR共享IF2并要求不可互相嵌套；多任务和嵌套中断宿主必须在HAL外提供仲裁。

## 冻结前门禁

已完成的软件门禁：

- 训练仓库通过submodule固定候选，P2-L01至P2-L07均已完成完整ELF/HEX构建；
- 已记录七个宿主镜像尺寸和P2-L06/P2-L07 CAN ISR静态调用链栈上界；
- 公共头文件C/C++ linkage、仓库边界和SPSC主机测试通过。

仍需目标硬件证据：

- 按[目标时序验证](hal_can_1_0_target_timing.md)使用目标周期计数确认RAM初始化和IF1/IF2等待上限；
- 使用当前候选逐Lab完成P2-L01至P2-L07上板回归；
- 对常用可测分支补充扩展帧TX、零DLC、队列满、Silent和Loopback+Silent验证；
- 更新最终能力矩阵、候选commit、上板工具链信息和未验证边界。

## 当前未验证边界

Bus-off动态进入、强制IF超时、External Loopback、长期压力和CANopenNode适配均不属于现有证据。它们不阻塞初版功能基线，但不得描述为量产验证通过。
