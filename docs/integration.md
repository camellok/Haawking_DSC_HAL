# 宿主工程集成

## 首次添加

```bash
git submodule add https://github.com/camellok/Haawking_DSC_HAL.git third_party/Haawking_DSC_HAL
git submodule update --init --recursive
```

把以下路径加入宿主构建：

```text
Include: third_party/Haawking_DSC_HAL/include
Source : third_party/Haawking_DSC_HAL/source/hxs320f28002x/hal_cputimer.c
```

宿主还必须提供目标匹配的`driverlib.h`和DriverLib实现。不要把`source`根目录整体加入递归编译。

## Platform接入

Platform静态创建`HAL_CPUTIMER_Obj`，把`baseAddress`设为实际CpuTimer实例，然后调用`HAL_CPUTIMER_init()`。时钟源、中断注册和ISR仍由platform处理。可以参考`reference/platform/launchboard_28002x`，但应按真实工程启动顺序调整。

## 固定和升级版本

子模块记录精确提交。发布后应检出标签对应提交，再在宿主仓库提交子模块指针。不要配置为自动跟随`main`。

升级流程：

1. 阅读目标版本CHANGELOG；
2. 更新子模块到指定标签；
3. 检查公共API兼容性；
4. 重新完成宿主编译和相关硬件回归；
5. 用独立宿主提交记录子模块升级和验证结果。
