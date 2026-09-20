# Haawking DSC HAL

> A reusable, target-oriented hardware abstraction library for Haawking DSC projects. Vendor DriverLib and device support are supplied by the consuming firmware project.

Haawking DSC HAL是面向真实固件项目维护的完整外设抽象库。仓库保存稳定的公共接口、芯片族目标实现、platform设计规范和验证记录；不打包DriverLib、device支持或完整应用例程。路线优先覆盖CpuTimer、ePWM、ADC、CLA、CAN、SCI和I2C，并按项目需求扩展其余外设。

> [!IMPORTANT]
> 当前状态为早期开发。CpuTimer接口与HXS320F28002x目标实现已经建立，但在宿主工程完成编译和上板验收前不会发布`v0.1.0`。

## 外设状态

| 外设 | 公共API | 目标实现 | 宿主编译 | 上板验证 |
|---|---|---|---|---|
| CpuTimer | 已实现 | HXS320F28002x DriverLib | 待验证 | 待验证 |
| ePWM | 进行中 | 未开始 | 未开始 | 未开始 |
| ADC | 规划中 | 未开始 | 未开始 | 未开始 |
| CLA | 规划中 | 未开始 | 未开始 | 未开始 |
| CAN | HAL_CAN-1.0候选 | HXS320F28002x候选实现 | 待宿主回归 | 既有Lab证据，冻结候选待回归 |
| SCI | 规划中 | 未开始 | 未开始 | 未开始 |
| I2C | 规划中 | 未开始 | 未开始 | 未开始 |

详细进度见[ROADMAP](ROADMAP.md)。

## 通用软件组件

| 组件 | 状态 | 动态内存 | 说明 |
|---|---|---|---|
| 泛型队列 | SPSC实现并有主机测试 | 不使用 | 调用者提供固定容量存储区，支持单核ISR生产/前台消费 |

泛型队列位于`source/common`，不依赖DriverLib或具体外设。API、错误语义、使用示例和并发边界见[软件队列说明](docs/software_queue.md)。

## 分层关系

```text
Application / Service
          |
          +------> platform（板级资源、时钟、IRQ绑定）
          |
          +------> HAL公共API
                         |
                  目标芯片实现
                         |
                 宿主提供DriverLib
```

- HAL描述外设能力、生命周期和错误语义。
- 每块板使用一套通用platform组合层，持有各HAL对象并统一绑定实例、时钟、中断和跨外设资源。
- 应用决定ISR放置、应用时基、调度周期和产品策略。

完整规则见[架构说明](docs/architecture.md)和[platform规范](docs/platform_guidelines.md)。

## 作为子模块集成

```bash
git submodule add https://github.com/camellok/Haawking_DSC_HAL.git third_party/Haawking_DSC_HAL
git submodule update --init --recursive
```

宿主工程需要：

1. 把`third_party/Haawking_DSC_HAL/include`加入头文件搜索路径；
2. 按需编译`source/common`中的目标无关组件，并且只编译一个目标芯片目录，例如`source/hxs320f28002x/hal_cputimer.c`；
3. 提供匹配目标芯片的`driverlib.h`、DriverLib实现和device启动代码；
4. 在platform中创建静态HAL对象并设置外设基地址；
5. 把子模块固定到已验证发布标签对应的提交。

更完整的接入和升级流程见[集成指南](docs/integration.md)。

## 仓库不包含

- Haawking DriverLib源码或静态库；
- device启动文件和链接脚本；
- CANopen等协议栈；
- 产品业务逻辑；
- 可独立构建的完整例程。

`reference/platform`中的代码展示整板资源所有权、初始化顺序和调用关系，可以复制并按项目需要改造。参考platform不会为每个HAL模块建立相互独立的板级封装。

## 协作与版本

公共API变更通过短期`codex/*`分支和Pull Request进入`main`。发布版本采用Semantic Versioning；`v0.1.0`之前的代码均视为开发状态。

提交要求见[CONTRIBUTING](CONTRIBUTING.md)，跨会话协作约定见[AGENTS](AGENTS.md)，版本变化见[CHANGELOG](CHANGELOG.md)。

## License

本项目采用[Apache License 2.0](LICENSE)。主动提交的贡献按同一许可证提供。
