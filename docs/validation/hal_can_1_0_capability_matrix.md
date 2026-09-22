# HAL_CAN-1.0能力与接口矩阵

> 候选目标：HXS320F28002x DCAN  
> 历史实验宿主：Haawking_DSC_Training_CANopen，P2-L01至P2-L07  
> 候选回归宿主：Haawking_DSC_Training_CANopen `3e8776f`，HAL `6495488`
> 回归日期：2026-09-22

## 接口必要性审计

| 接口 | 初版必要性 | 后端状态 | 既有证据或冻结边界 |
|---|---|---|---|
| `HAL_CAN_init()` | 必须 | 已实现 | 候选R01至R12正常初始化路径通过；有界消息RAM初始化未做强制超时注入 |
| `HAL_CAN_softReset()` | 保留能力契约 | `UNSUPPORTED` | 独立复位的邮箱、队列和诊断保留语义未定义；无副作用，不宣称芯片无复位能力 |
| `HAL_CAN_start()` | 必须 | 已实现 | L01至L07 |
| `HAL_CAN_stop()` | 必须 | 已实现 | 候选R11受控恢复主链路通过 |
| `HAL_CAN_setAutoRetransmission()` | 必须 | 已实现 | L07 ACK故障、单次探测与恢复 |
| `HAL_CAN_cancelAllTransmitRequests()` | 必须 | 已实现 | Bus-off恢复分支已实现，动态Bus-off未实测 |
| `HAL_CAN_abortTx()` | 必须 | 已实现 | L07 Error Passive单次探测路径 |
| `HAL_CAN_getTxPending()` | 必须 | 已实现 | L07使用TXRQ判定发送挂起 |
| `HAL_CAN_configureRx()` | 必须 | 已实现 | L02、L03、L04、L05、L06 |
| `HAL_CAN_configureTx()` | 必须 | 已实现 | L01、L03、L06、L07 |
| `HAL_CAN_configureRemoteRequest()` | Classic CAN可选能力 | 已实现 | L05标准RTR请求；扩展RTR未专项验证 |
| `HAL_CAN_requestRemote()` | Classic CAN可选能力 | 已实现 | L05标准RTR请求 |
| `HAL_CAN_configureRemoteResponse()` | Classic CAN可选能力 | 已实现 | L05硬件自动响应 |
| `HAL_CAN_send()` | 必须 | 已实现 | 候选R01至R03 polling及R09中断事件应用路径通过 |
| `HAL_CAN_updateRemoteResponse()` | 自动响应配套能力 | 已实现 | L05运行期payload更新 |
| `HAL_CAN_receive()` | 必须 | 已实现 | L02至L05 polling；中断模式下明确拒绝前台竞争IF2 |
| `HAL_CAN_configureInterrupts()` | 产品应用必须 | 已实现 | L06、L07；使用调用方提供的SPSC存储 |
| `HAL_CAN_enableInterrupts()` | 产品应用必须 | 已实现 | L06、L07 |
| `HAL_CAN_disableInterrupts()` | 产品应用必须 | 已实现 | 生命周期和队列重绑定契约已有代码，专项回归待完成 |
| `HAL_CAN_enableDiagnosticInterrupts()` | 诊断恢复必须 | 已实现 | L07恢复后重新交还状态读取所有权 |
| `HAL_CAN_disableDiagnosticInterrupts()` | 诊断恢复必须 | 已实现 | L07 ISR故障交接 |
| `HAL_CAN_getDiagnosticFaultEvent()` | 诊断恢复必须 | 已实现 | L07 ISR到前台事件锁存 |
| `HAL_CAN_getRxEvent()` | 中断RX必须 | 已实现 | L06、L07 |
| `HAL_CAN_getTxCompleteEvent()` | 中断TX必须 | 已实现 | L06、L07 |
| `HAL_CAN_captureDiagnostics()` | 产品诊断必须 | 已实现 | L07 ACK、Error Passive和恢复 |
| `HAL_CAN_process()` | 前台诊断所有权必须 | 已实现 | L07诊断源屏蔽后的周期刷新 |
| `HAL_CAN_getDiagnostics()` | 可观测性必须 | 已实现 | L03、L06、L07；一致性快照由CAN全局线门控保护 |

结论：公共头文件没有“已声明但空实现”的接口。唯一固定返回`HAL_STATUS_UNSUPPORTED`的是`HAL_CAN_softReset()`，保留它是为了稳定跨后端能力探测，而不是等待本目标补代码。删除该接口会迫使上层通过芯片宏判断后端，不利于移植。

## 功能验证状态

| 能力 | 实现 | 历史上板证据 | 候选回归 |
|---|---|---|---|
| 标准帧polling TX/RX | 完成 | L01、L02 | R01 TX通过；R04目标帧RX通过，错误ID拒绝和`EMPTY`不改写未单独留证 |
| 内部Loopback | 完成 | L03 | R05通过，256/256且失败数0 |
| 标准/扩展ID与过滤 | 完成 | L04 | R07通过，标准精确、标准范围、扩展精确、边界拒绝和格式隔离均在同一序列留证 |
| 多消息对象 | 完成 | L04 | R07三个RX对象分流通过 |
| 标准RTR请求与自动响应 | 完成 | L05 | R08请求、人工数据响应、硬件自动响应和payload更新通过 |
| RX/TX双中断线与SPSC事件 | 完成 | L06 | R09连续125组事件交接通过，相关溢出、超时和意外中断计数为0 |
| ACK错误、Error Passive和受控探测恢复 | 完成 | L07 | R11主链路通过：ACK、TEC 128、Error Passive、单次探测和恢复到Error Active |
| Bus-off动态进入与恢复 | 完成 | 未实测 | 保持未验证 |
| Silent | 完成 | 未形成独立节点模式证据 | 候选未单独留证，作为明确边界保留 |
| Loopback+Silent | 完成 | 未验证 | R06通过，256/256且外部总线无帧 |
| External Loopback | 完成 | 未验证 | 可保留未验证 |
| 零DLC数据帧 | 完成 | 未形成专项证据 | R02通过，标准ID `0x123`、DLC 0 |
| 扩展ID TX | 完成 | RX过滤已有扩展帧证据，TX未专项验证 | R03通过，扩展ID `0x18FF50E5`、DLC 8 |
| 队列满时丢弃最新事件并计数 | 完成 | 未强制触发 | R10由用户决定不执行，作为明确边界保留 |
| IF1/IF2强制超时 | 完成 | 未故障注入 | 可保留未验证 |
| 长时间满载与突发压力 | 完成基础能力 | P2-L08延期 | 冻结后增强验证 |
| CANopenNode `CO_driver` | 不属于HAL实现 | 未开始 | P3完成 |

## 冻结判定

- 候选回归已证明标准/扩展TX、零DLC、目标帧polling RX、Loopback、Loopback+Silent、过滤、多对象、远程帧、中断事件及Error Passive恢复主链路。
- R04的错误ID拒绝与`EMPTY`不改写、standalone Silent、R10队列满、Bus-off动态进入、强制IF超时和External Loopback没有当前候选的直接证据，冻结时必须作为“已实现、未验证”逐项列出。
- R10由用户明确决定不执行；该决定不等同于验证通过，也不能用正常连续收发时溢出计数为0替代队列满策略证据。
- 正常回归中P2-L01至P2-L07的`transactionTimeoutCount`和`unexpectedInterruptCount`均为0；独立目标时序门禁也已通过，现有IF事务1000次和消息RAM初始化100000次保守等待上限保持不变。临时探针移除后的发布构建、目标ABI、严格告警和ISR静态栈门禁已重新通过。
- P2-L08长期压力和P3协议栈适配不是`HAL_CAN-1.0`候选合入的前置条件，不得反向把协议策略加入HAL。

