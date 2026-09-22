# HAL_CAN-1.0目标时序验证

> 状态：HXS320F28002x上板采集通过；临时探针已移除，清理后发布门禁通过
> 目的：为IF1、IF2和消息RAM初始化的有界等待确定发布参数

## 测量原则

- 使用RISC-V `mcycle`自由运行计数器，不启用新的CpuTimer中断；
- 使用与发布工程相同的系统时钟、编译器、优化等级和Flash执行配置；
- 先测量两次连续读取`mcycle`的空测量开销，结果统一扣除该开销；
- 记录最大值、最小值、样本数和测试模式，不只记录平均值；
- 测量代码属于验证工程，不进入HAL公共接口和正式运行路径。

## 测量对象

| 对象 | 建议激励 | 记录内容 |
|---|---|---|
| 消息RAM初始化 | 连续冷启动和调试器复位各100次 | `HAL_CAN_init()`中RAM_INIT置位到完成的周期数 |
| IF1数据发送事务 | Normal模式有ACK，连续发送至少10000帧 | IF1命令提交到BUSY清零的最大周期数 |
| IF1远程请求与响应更新 | Loopback或可控外部节点，各至少1000次 | 每种IF1事务的最大周期数 |
| IF2 polling接收 | Loopback连续收发至少10000帧 | 快照读取和NEWDAT清除两个事务的最大周期数 |
| IF2 ISR接收 | 外部节点按当前Lab队列可持续速率发送至少10000帧 | ISR内每个IF2事务及ISR入口到退出的最大周期数 |
| IF2 TX完成清除 | 开启TX完成中断并发送至少10000帧 | 清除对象中断事务和Line 1 ISR总周期数 |

Normal、Loopback和Loopback+Silent至少覆盖前两种；Silent没有发送事务，不用于评价IF1发送完成时间。External Loopback在缺少稳定外部回路时继续标为未验证。

## 临时观测量

上板采集时，临时验证工程在定义`HAL_CAN_TARGET_TIMING_ENABLE`后提供了以下全局`volatile uint32_t`观测量。它们没有加入公共`hal_can.h`或`HAL_CAN_Obj`，验收后已随临时探针一起移除，不进入发布源码和镜像：

- `canTimingReadOverheadCycles`；
- `canRamInitCyclesMin`、`canRamInitCyclesMax`、
  `canRamInitSampleCount`和`canRamInitBusyPollsMax`；
- `canIf1TransactionCyclesMin`、`canIf1TransactionCyclesMax`、
  `canIf1TransactionSampleCount`和`canIf1BusyPollsMax`；
- `canIf2TransactionCyclesMin`、`canIf2TransactionCyclesMax`、
  `canIf2TransactionSampleCount`和`canIf2BusyPollsMax`；
- `canInterruptTimingReadOverheadCycles`；
- `canInterrupt0CyclesMin`、`canInterrupt0CyclesMax`和
  `canInterrupt0SampleCount`；
- `canInterrupt1CyclesMin`、`canInterrupt1CyclesMax`和
  `canInterrupt1SampleCount`。

使用无符号减法计算`endCycle - startCycle`，可正确处理32位计数器单次回绕。一次被测区间必须远小于计数器完整回绕周期。

IF1/IF2周期覆盖每次有界等待调用：空闲前置检查通常形成最小值，命令提交后的BUSY等待形成较大样本，发布判定使用最大值。ISR周期从C函数体首个观测点量到清除外设与PIE标志之后，不包含编译器生成的入口保存和出口恢复；完整ISR预算必须同时参考静态调用链栈报告和最终反汇编，不能把该数值描述为硬件中断响应延迟。

## 上板采集使用的临时构建

P2-L06采集时临时启用了`HAL_CAN_TARGET_TIMING_ENABLE`。该构建具有以下验证专用行为：

- platform在正常`HAL_CAN_init()`前重复执行100次有界消息RAM初始化，连同最终正常初始化，`canRamInitSampleCount`应为101；
- HAL目标私有实现记录全部IF1/IF2等待；
- Line 0和Line 1 ISR分别记录C函数体周期；
- 公共HAL接口、对象布局、队列深度和应用收发语义不变。

该临时构建通过了目标ABI、严格告警以及P2-L01至P2-L07完整ELF/HEX链接。P2-L06探针镜像的`text/data/bss/total`为`22220/204/352/22776`字节；它只用于本次验证，不替代关闭探针后的发布尺寸。采集完成后，探针宏、目标源码观测代码、ISR观测代码、重复RAM初始化激励和脚本开关均已删除；若未来需要重复采样，应从本次候选历史恢复同一套隔离探针并重新执行代码审查，不应在发布配置中常驻。

## 上板采集步骤

1. 在IDE中对P2-L06执行Clean Project和Build Project，确认编译命令包含`-DHAL_CAN_TARGET_TIMING_ENABLE`并下载本次ELF；
2. CAN分析仪使用Normal模式参与ACK，以500 kbit/s周期发送标准数据帧ID `0x123`、DLC 8，建议周期5 ms；DSP应回发标准数据帧ID `0x456`；
3. 运行到`rxEventCount`、`txSubmitCount`和`txCompleteCount`均不小于10000，并确认三者保持一致；
4. 在同一张或连续截图中保存全部周期变量、`canHandle->canDiagnostics`和三个应用计数；
5. 另保存分析仪中ID `0x123`/`0x456`、DLC、数据和发送周期；
6. 把原始截图放入训练仓库
   `Docs/lesson_notes/02_can_hal/hal_can_1_0_candidate_regression/target_timing/`；
7. 正常结束后删除P2-L06工程中的临时宏和重复初始化代码，并重新运行不带开关的发布门禁。

160 MHz系统时钟下，周期数换算为微秒使用`cycles / 160`。判定时保留原始周期整数，换算值只用于可读性说明。

## 2026-09-22实测结果

测试使用P2-L06探针构建、160 MHz系统时钟、500 kbit/s CAN和Normal模式分析仪ACK。分析仪以5 ms周期发送标准数据帧ID `0x123`、DLC 8，DSP通过中断队列回发标准数据帧ID `0x456`、DLC 8。

| 测量对象 | 最小周期 | 最大周期 | 样本数 | 最大BUSY轮询 | 最大时间 |
|---|---:|---:|---:|---:|---:|
| 消息RAM初始化 | 285 | 285 | 101 | 12 | 1.78125 us |
| IF1有界等待 | 13 | 609 | 1859832 | 0 | 3.80625 us |
| IF2有界等待 | 10 | 12 | 2303095 | 0 | 0.075 us |
| Line 0 ISR函数体 | 915 | 924 | 460603 | 不适用 | 5.775 us |
| Line 1 ISR函数体 | 366 | 372 | 460606 | 不适用 | 2.325 us |

ISR探针两次计数器读取的最小开销为2周期。当前截图没有单独展示HAL事务探针的`canTimingReadOverheadCycles`，但表中的IF与RAM统计值已经由探针在目标端完成开销扣除；不从截图推断该独立变量的具体值。

IF1与IF2的最大BUSY轮询均为0，表示目标硬件在CPU执行第一次BUSY条件采样前已经完成正常事务，并不表示相关路径未执行。IF1在前台执行，609周期最大值可包含CAN ISR抢占；该值不能反推为609周期的硬件BUSY时间。IF2由不可相互嵌套的CAN ISR串行占有，其10至12周期分布更接近一次空闲检查的固定开销。

应用截图取得`rxEventCount=460595`、`txSubmitCount=460596`、`txCompleteCount=460597`，诊断截图稍后取得`rxFrameCount=467210`、`txRequestCount=467212`、`txCompleteCount=467213`。这些Live View快照在程序持续运行期间分时读取，相邻计数允许存在一至三次在途事件；两组均远高于10000样本要求，且`frameErrorCount`、RX硬件溢出、RX队列溢出、TX事件队列溢出、事务超时和意外中断均为0。

原始证据位于训练仓库：

- `Docs/lesson_notes/02_can_hal/hal_can_1_0_candidate_regression/target_timing/application_counts.png`；
- `Docs/lesson_notes/02_can_hal/hal_can_1_0_candidate_regression/target_timing/diagnostic_counts.png`；
- `Docs/lesson_notes/02_can_hal/hal_can_1_0_candidate_regression/target_timing/testing_analysis.png`。

## 验收判定

本轮正常路径目标时序验收通过：样本规模、收发语义、诊断计数和正常ISR函数体周期均满足本候选验证要求。现有IF等待上限1000次、消息RAM初始化上限100000次远高于本轮正常样本，且正常运行不为未执行的剩余迭代付出时间成本；单次目标板测量不足以证明主动收紧上限在全部电压、温度、Flash等待和后续同族器件上更可靠，因此HAL_CAN-1.0初版保留当前保守上限。

该结论不把强制BUSY超时路径或完整ISR硬件入口/出口描述为已动态验证。最终产品若具有比本参考工程更严格的中断驻留预算，仍应结合其优先级嵌套和反汇编重新评估故障路径。临时探针和P2-L06重复初始化已经移除，关闭探针后的发布构建、目标ABI、严格告警和静态栈门禁均已重新通过。

## 发布阈值判定

1. 保留每类事务的最坏实测值，不用平均值确定阈值；
2. 把编译后的BUSY等待循环单次迭代周期和函数固定开销纳入换算；
3. 发布迭代上限必须明显高于全部正常样本，同时满足最坏ISR驻留预算；
4. 若拟定上限小于最坏实测值的4倍，必须说明时钟、Flash等待状态和中断抢占的覆盖依据；
5. 若1000次IF等待或100000次RAM初始化等待造成不可接受的故障驻留时间，应降低上限或改用基于周期计数的deadline；
6. 最终数值、换算过程、工具链和测试证据写回`hal_can_1_0_review.md`，再移除临时观测代码。

## 通过条件

- 所有正常样本均在发布阈值内，`transactionTimeoutCount`保持0；
- RX/TX事件计数与应用消费计数一致，队列溢出和意外中断符合测试预期；
- 最坏Line 0/Line 1 ISR周期满足宿主系统的中断延迟预算；
- 发布阈值有明确测量依据，且强制超时路径仍保持“已实现、未注入验证”或另有独立故障注入证据。

