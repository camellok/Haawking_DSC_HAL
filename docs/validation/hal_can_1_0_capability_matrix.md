# HAL_CAN-1.0能力与接口矩阵

> 候选目标：HXS320F28002x DCAN  
> 历史实验宿主：Haawking_DSC_Training_CANopen，P2-L01至P2-L07  
> 说明：历史Lab证据证明迁移来源的对应行为，不替代候选提交的P2-L11回归

## 接口必要性审计

| 接口 | 初版必要性 | 后端状态 | 既有证据或冻结边界 |
|---|---|---|---|
| `HAL_CAN_init()` | 必须 | 已实现 | L01至L07均经过初始化；候选新增有界消息RAM初始化，待回归 |
| `HAL_CAN_softReset()` | 保留能力契约 | `UNSUPPORTED` | 独立复位的邮箱、队列和诊断保留语义未定义；无副作用，不宣称芯片无复位能力 |
| `HAL_CAN_start()` | 必须 | 已实现 | L01至L07 |
| `HAL_CAN_stop()` | 必须 | 已实现 | L07受控恢复路径；候选回归待完成 |
| `HAL_CAN_setAutoRetransmission()` | 必须 | 已实现 | L07 ACK故障、单次探测与恢复 |
| `HAL_CAN_cancelAllTransmitRequests()` | 必须 | 已实现 | Bus-off恢复分支已实现，动态Bus-off未实测 |
| `HAL_CAN_abortTx()` | 必须 | 已实现 | L07 Error Passive单次探测路径 |
| `HAL_CAN_getTxPending()` | 必须 | 已实现 | L07使用TXRQ判定发送挂起 |
| `HAL_CAN_configureRx()` | 必须 | 已实现 | L02、L03、L04、L05、L06 |
| `HAL_CAN_configureTx()` | 必须 | 已实现 | L01、L03、L06、L07 |
| `HAL_CAN_configureRemoteRequest()` | Classic CAN可选能力 | 已实现 | L05标准RTR请求；扩展RTR未专项验证 |
| `HAL_CAN_requestRemote()` | Classic CAN可选能力 | 已实现 | L05标准RTR请求 |
| `HAL_CAN_configureRemoteResponse()` | Classic CAN可选能力 | 已实现 | L05硬件自动响应 |
| `HAL_CAN_send()` | 必须 | 已实现 | polling与中断实验均使用；候选配置路径待回归 |
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
| 标准帧polling TX/RX | 完成 | L01、L02 | 待P2-L11 |
| 内部Loopback | 完成 | L03 | 待P2-L11 |
| 标准/扩展ID与过滤 | 完成 | L04 | 待P2-L11 |
| 多消息对象 | 完成 | L04 | 待P2-L11 |
| 标准RTR请求与自动响应 | 完成 | L05 | 待P2-L11 |
| RX/TX双中断线与SPSC事件 | 完成 | L06 | 待P2-L11 |
| ACK错误、Error Passive和受控探测恢复 | 完成 | L07 | 待P2-L11 |
| Bus-off动态进入与恢复 | 完成 | 未实测 | 保持未验证 |
| Silent | 完成 | 未形成独立节点模式证据 | 补充测试 |
| Loopback+Silent | 完成 | 未验证 | 补充测试 |
| External Loopback | 完成 | 未验证 | 可保留未验证 |
| 零DLC数据帧 | 完成 | 未形成专项证据 | 补充测试 |
| 扩展ID TX | 完成 | RX过滤已有扩展帧证据，TX未专项验证 | 补充测试 |
| 队列满时丢弃最新事件并计数 | 完成 | 未强制触发 | 补充测试 |
| IF1/IF2强制超时 | 完成 | 未故障注入 | 可保留未验证 |
| 长时间满载与突发压力 | 完成基础能力 | P2-L08延期 | 冻结后增强验证 |
| CANopenNode `CO_driver` | 不属于HAL实现 | 未开始 | P3完成 |

## 冻结判定

- 常用且可稳定复现的分支必须在P2-L11补充候选回归：标准TX/RX、Loopback、过滤、中断事件、错误诊断、扩展TX、零DLC、队列满、Silent和Loopback+Silent。
- Bus-off动态进入、强制IF超时和External Loopback受测试条件限制，可以作为“已实现、未验证”保留，但发布说明必须逐项列出。
- P2-L08长期压力和P3协议栈适配不是`HAL_CAN-1.0`候选合入的前置条件，不得反向把协议策略加入HAL。

