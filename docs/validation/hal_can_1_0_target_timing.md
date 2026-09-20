# HAL_CAN-1.0目标时序验证

> 状态：待HXS320F28002x上板执行  
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

验证工程可在目标私有事务附近增加以下全局`volatile uint32_t`观测量，但不得加入公共`hal_can.h`或`HAL_CAN_Obj`：

- `canRamInitCyclesMax`；
- `canIf1TransactionCyclesMax`；
- `canIf2TransactionCyclesMax`；
- `canInterrupt0CyclesMax`；
- `canInterrupt1CyclesMax`；
- 对应的样本计数。

使用无符号减法计算`endCycle - startCycle`，可正确处理32位计数器单次回绕。一次被测区间必须远小于计数器完整回绕周期。

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

