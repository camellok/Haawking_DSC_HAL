# LaunchBoard 28002x参考platform

本目录展示一块目标板如何通过单一platform组合入口绑定多个HAL模块。它是可复制改造的架构参考，不是CpuTimer专用封装、稳定公共API或可独立构建的完整例程。

## 文件职责

| 文件 | 职责 |
|---|---|
| `platform_config.h` | 集中外设实例、IRQ、ACK组、时钟源等资源连接宏，并声明用户配置实例 |
| `platform_config.c` | 定义用户可直接调整的全局HAL配置实例 |
| `platform.h` | 向应用和服务暴露整板初始化、毫秒tick和原始时间戳等板级能力 |
| `platform.c` | 私有持有HAL运行对象，使用配置层提供的资源和参数组合整个平台 |

随着HAL模块增加，可以将`platform.c`内部实现拆成`platform_pwm.c`、`platform_adc.c`、`platform_can.c`等私有职责文件，但这些文件仍共同实现同一个板级platform。应用不应看到一套彼此独立的“CpuTimer platform”“ADC platform”或“CAN platform”。

## 当前资源分配

| 资源 | 用途 | 所有者 |
|---|---|---|
| CpuTimer0 | 1 ms platform时间基准 | platform持有对象、ISR和tick状态 |
| CpuTimer1 | 未分配 | 留给实际项目统一分配 |
| CpuTimer2 | 32位自由运行递减时间戳 | platform持有对象并选择时钟源 |
| PIE Group 1 / Timer0 IRQ | 时间基准中断 | platform注册ISR并完成ACK |

当前实现仅因仓库已完成CpuTimer HAL而组合时间服务。后续模块进入仓库时，同一platform应继续维护一张整板资源表，例如：

- ePWM实例、输出GPIO、同步链、Trip输入和ADC SOC触发；
- ADC实例、SOC编号、模拟通道、PPB、中断和DMA目标；
- CLA任务、消息RAM、触发源和任务完成中断；
- CAN、SCI、I2C实例、GPIO、中断、DMA和缓冲区；
- CMPSS、XBAR、ePWM Trip之间的保护链路。

资源连接和HAL配置采用两种表达：

- 可在预处理期确定的外设基地址、GPIO、IRQ、ACK组、时钟源和XBAR选择放在`platform_config.h`；
- 需要传给HAL初始化接口的配置实例放在`platform_config.c`，使用全局静态存储期，用户在构建前直接修改初始化值；
- HAL运行对象和platform内部状态保留在`platform.c`，不向应用暴露。

## 启动顺序

参考`PLATFORM_init()`假设宿主已完成device时钟和中断控制器基础初始化，但尚未开启全局中断。整板platform应按硬件依赖安排初始化：

1. 核对时钟树并配置pinmux、输入资格和XBAR；
2. 建立保护链路，使功率输出保持在安全状态；
3. 初始化通信、PWM、ADC、CLA和DMA等platform持有对象；
4. 注册ISR并清理陈旧外设标志和PIE ACK；
5. 启动时间服务和必须运行的外设；
6. 返回宿主启动代码，由其决定全局中断和功率输出的最终开启时机。

## 时间服务示例

参考配置假设系统时钟为160 MHz。移植时必须修改`platform_config.h`中的资源宏和时钟值，并在`platform_config.c`核对配置实例，然后用宿主工程实际时钟验证整除关系和1 ms周期。

Timer2提供原始递减时间戳。若一次测量最多跨越一次回绕，可使用无符号减法：

```c
elapsedTicks = startTimestamp - endTimestamp;
```

更长时间累计、物理时间换算、调度器和产品超时策略属于platform上层服务或应用职责。
