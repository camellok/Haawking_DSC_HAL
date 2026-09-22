# Changelog

本项目采用[Semantic Versioning](https://semver.org/)；正式发布前的变化记录在`Unreleased`。

## Unreleased

### Added

- 建立HAL仓库结构、协作规范、Roadmap和GitHub工作流。
- 增加跨外设公共`HAL_Status_t`。
- 增加CpuTimer公共API和HXS320F28002x DriverLib目标实现。
- 增加LaunchBoard 28002x通用参考platform，以单一组合入口管理当前CpuTimer时间服务并预留后续外设集成位置。
- 展开ePWM、ADC、CLA、CAN、SCI、I2C及其他外设的分阶段Roadmap。
- 为仓库自建C源文件和头文件增加统一文件说明块，并将规则加入AGENTS。
- 将参考platform的外设连接宏集中到`platform_config.h`，并在`platform.c`文件作用域定义用户可修改的CpuTimer配置实例。
- 将Timer0 ISR改为宿主提供的外部符号，移除platform维护的毫秒计数，并明确应用时基属于应用层。
- 增加目标无关的固定容量泛型软件队列、`HAL_STATUS_FULL`状态和主机单元测试。
- 将泛型队列的公开存储指针改为`void *`，仅在实现内使用`unsigned char *`计算元素槽位。
- 增加Classic CAN公共API和HXS320F28002x DCAN目标实现候选，覆盖polling、中断队列、远程帧、诊断和受控恢复原语。
- 将泛型队列升级为Sequence型SPSC实现，并增加无符号Sequence回绕测试。
- 为DCAN消息RAM初始化和IF1/IF2消息对象事务增加有界等待与超时返回。
- 增加公共HAL头文件的C++语法门禁，并补齐CAN公共头的C linkage保护。
- 收紧CAN实例零初始化与重初始化契约，硬件初始化超时后保留安全的未初始化软件状态。

### Validation

- 仓库静态检查：通过。
- 使用最小DriverLib声明进行GCC严格语法检查：通过。
- 训练仓库P2-L01至P2-L07完整ELF/HEX构建：通过。
- P2-L06与P2-L07 CAN ISR静态调用链栈上界：已记录。
- HXS320F280025C候选上板回归：标准/扩展TX、零DLC、polling RX、Loopback、Loopback+Silent、多对象过滤、远程帧、中断事件及Error Passive受控恢复主链路通过。
- P2-L01至P2-L07正常回归结束时的事务超时与意外中断计数均为0。
- 目标周期计数门禁：160 MHz上板采样通过；消息RAM初始化最大285周期，IF1/IF2最大609/12周期，Line 0/Line 1 ISR函数体最大924/372周期；采样期间事务超时、意外中断、帧错误和队列溢出均为0，现有保守等待上限保持不变。
- 临时时序探针和宿主重复RAM初始化激励移除后，发布态七Lab完整构建、目标ABI、严格告警、ISR静态栈、仓库边界、SPSC队列及C/C++公共头链接门禁重新通过。
- standalone Silent、队列满丢弃策略、动态Bus-off、强制IF超时、External Loopback和长期压力保持明确未验证；其中队列满测试由宿主项目决定不执行。
