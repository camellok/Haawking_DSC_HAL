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
Common : third_party/Haawking_DSC_HAL/source/common/hal_queue.c（按需）
```

Classic CAN目标还需按需加入：

```text
Source : third_party/Haawking_DSC_HAL/source/hxs320f28002x/hal_can.c
Source : third_party/Haawking_DSC_HAL/source/hxs320f28002x/hal_can_hw.c
Common : third_party/Haawking_DSC_HAL/source/common/hal_queue.c（中断事件模式）
```

`hal_can_hw.h`是同一目标目录内的私有集成头，只供目标实现和宿主ISR使用，不应加入应用公共接口。

CAN运行期IF1接口不是多任务可重入API；RTOS宿主需要在服务层串行化发送、取消和远程帧更新。Line 0 RX与Line 1 TX完成ISR共同使用IF2，向量配置必须禁止这两个ISR相互嵌套。普通单核裸机中断入口不主动重新开放全局中断时无需额外锁。

宿主还必须提供目标匹配的`driverlib.h`和DriverLib实现。不要把`source`根目录整体加入递归编译。

`source/common`中的文件不依赖DriverLib，可以由所有目标共用。只加入实际使用的组件；泛型队列的存储区由宿主静态分配，跨ISR与主循环共享时由宿主负责同步。

## Platform接入

Platform静态创建各外设HAL对象，绑定实际实例，并通过单一整板初始化入口按依赖顺序完成组合。外设基地址、GPIO、IRQ、ACK组和时钟源等连接宏集中在`platform_config.h`，用户可修改的全局HAL配置实例在platform实现文件中以文件作用域定义。以CpuTimer为例，`platform.c`设置`HAL_CPUTIMER_Obj.baseAddress`后，把全局配置实例传给`HAL_CPUTIMER_init()`并注册宿主提供的外部ISR。ISR可以放在`main.c`或统一ISR文件；1 ms应用时基由应用维护，platform只提供硬件标志清理和PIE ACK接口。可以参考`reference/platform/launchboard_28002x`，但必须按真实工程的资源表和启动顺序调整。

## 固定和升级版本

子模块记录精确提交。发布后应检出标签对应提交，再在宿主仓库提交子模块指针。不要配置为自动跟随`main`。

升级流程：

1. 阅读目标版本CHANGELOG；
2. 更新子模块到指定标签；
3. 检查公共API兼容性；
4. 重新完成宿主编译和相关硬件回归；
5. 用独立宿主提交记录子模块升级和验证结果。
