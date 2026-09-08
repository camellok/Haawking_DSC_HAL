# LaunchBoard 28002x CpuTimer参考绑定

本目录展示platform如何组合CpuTimer HAL，不是可独立运行的例程，也不是稳定公共API。

参考绑定分配CpuTimer0作为1 ms系统tick，CpuTimer2作为32位自由运行递减计数器，CpuTimer1保持未分配。实际项目可以改变实例、频率、中断优先级和接口名称，但应继续遵守以下边界：

- platform持有`HAL_CPUTIMER_Obj`并设置外设基地址；
- platform选择Timer2时钟源并完成中断注册和PIE ACK；
- HAL只配置CpuTimer外设内部行为；
- 应用只读取platform提供的时间服务，不直接使用DriverLib；
- 初始化前，宿主工程已经完成device和中断控制器初始化；
- 全局中断的最终开启时机由宿主工程顶层启动流程决定。

参考配置假设系统时钟为160 MHz。移植时必须将`PLATFORM_CPUTIMER_SYSTEM_CLOCK_HZ`替换为目标固件的实际时钟，并验证整除关系和实际tick周期。

Timer2耗时测量使用递减计数。若一次测量最多跨越一次回绕，可以使用无符号减法计算：

```c
elapsedTicks = startCount - endCount;
```

更长的统计窗口、微秒换算和回绕累计属于platform或调度服务职责。
