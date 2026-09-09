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
- 将参考platform的外设连接宏集中到`platform_config.h`，并把用户可修改的CpuTimer配置实例移到`platform_config.c`全局定义。
- 将Timer0 ISR改为宿主提供的外部符号，移除platform维护的毫秒计数，并明确应用时基属于应用层。

### Validation

- 仓库静态检查：通过。
- 使用最小DriverLib声明进行GCC严格语法检查：通过。
- 宿主工程编译：待验证。
- HXS320F280025C上板测试：待验证。
