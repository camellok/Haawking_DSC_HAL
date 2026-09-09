# Platform设计规范

Platform是目标硬件的组合与绑定层。它描述“这个固件使用哪个资源以及如何连接”，HAL描述“该外设能够完成什么操作”。

Platform负责：

- HAL对象的静态存储和外设实例映射；
- 系统时钟和外设时钟源；
- GPIO、pinmux、pad和输入资格；
- 中断注册、优先级、PIE ACK和全局中断启动顺序；
- DMA、XBAR和外部器件控制；
- 固定目标的HAL配置与安全初始化顺序。

每块参考板应提供一套通用platform组合入口，例如`platform.h`、`platform_config.h`和`platform.c`。随着外设增加，可以把内部实现拆分成多个私有源文件，但它们共同维护同一张板级资源表和同一条启动链，不为每个HAL模块创建相互独立的platform层。

建议的组合顺序：

1. 时钟假设、pinmux、输入资格和XBAR；
2. CMPSS、Trip等硬件保护链路；
3. ePWM、ADC、CLA、通信和DMA等HAL对象；
4. ISR注册、陈旧标志清理和PIE ACK；
5. platform时间服务与必要外设启动；
6. 返回顶层启动代码，由其决定全局中断和产品输出的最终开启。

Platform不应：

- 机械转发每个HAL运行接口；
- 把协议和产品状态机塞入板级代码；
- 修改HAL对象内部状态；
- 假设reference platform必须被每个项目原样复制。
- 向应用暴露HAL对象存储、DriverLib实例号或PIE ACK细节。

参考代码必须写明时钟、实例、中断和调用前置条件。实际项目可以采用不同文件结构，只要所有权和依赖方向保持清晰。
