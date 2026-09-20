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

### Validation

- 仓库静态检查：通过。
- 使用最小DriverLib声明进行GCC严格语法检查：通过。
- 宿主工程编译：待验证。
- HXS320F280025C上板测试：待验证。
