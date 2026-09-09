# 项目协作与验收规范

本文件适用于仓库中的HAL源码、参考platform、文档、验证记录和GitHub维护。用户最新明确要求具有最高优先级。

## 1. 项目边界

- 仓库维护公共HAL接口、目标芯片实现、架构规范、参考platform和验证记录。
- 不复制DriverLib、device支持、静态库、链接文件、协议栈、产品逻辑或完整例程。
- 公共头文件不得包含DriverLib类型或头文件；目标源码和参考platform可以使用宿主提供的DriverLib。
- 一个固件目标必须显式选择一个芯片族源码目录，不得通配编译所有目标实现。
- HAL负责单个外设能力；platform负责实例、时钟、引脚、IRQ绑定、DMA、对象存储和启动顺序；应用负责业务、调度策略和应用时基等行为。
- `reference/platform`用于说明架构和集成方法，不属于稳定公共API。

## 2. 协作职责

- 用户负责需求取舍、硬件环境、宿主工程集成、编译下载和上板结果确认。
- Codex负责仓库组织、HAL实现、代码审查、文档、Issue、PR、版本准备和兼容记录。
- Codex可以在已授权范围内维护仓库文件、功能分支、Pull Request、标签、Issue和GitHub主页。
- 发布版本前必须满足对应验收条件；用户确认上板测试不等于截图已经证明所有测试边界。

## 3. 开发工作流

每个外设按以下顺序推进：

1. 在Issue或Roadmap中确定能力和职责边界；
2. 冻结本阶段公共接口和失败语义；
3. 在目标芯片目录实现硬件适配；
4. 完成仓库静态检查；
5. 在宿主工程编译；
6. 完成上板测试并记录证据；
7. 更新兼容矩阵、验证记录和CHANGELOG；
8. 通过Pull Request合入`main`；
9. 满足milestone后创建版本标签和GitHub Release。

开发使用短期`codex/*`分支。`main`只接收通过检查的Pull Request。破坏公共API兼容性的修改必须在PR和版本记录中明确标注。

## 4. C代码规范

- 新建或迁入仓库自主管理的`.h`和`.c`文件时，文件顶部必须先保留`SPDX-License-Identifier: Apache-2.0`，并紧随其后使用统一说明块；`File`和`Description`按文件实际内容填写，其余字段保持一致：

```c
/******************************************************************************
 * Copyright (c) 2019-2026, Beijing Haawking Technology Co., Ltd
 *
 * Author: Silin Luo
 * Email : silin.luo@mail.haawking.com
 * File  : <file_name>
 * Description: <file_description>
 ******************************************************************************/
```

- 使用4个空格缩进，函数和控制块的大括号另起一行，所有控制块均使用大括号。
- 文件使用lower snake case；公共函数采用模块前缀和动词短语；宏使用带模块前缀的UPPER_SNAKE_CASE。
- 带单位的配置量在名称中标注单位；物理值与寄存器编码必须明确区分。
- 运行期间不动态分配内存；对象由platform或宿主静态分配。
- 初始化先完整验证参数，再修改硬件。验证失败不得留下部分配置。
- ISR保持有界，只处理必要硬件事件、数据交接、标志清除和中断ACK。
- ISR的物理文件位置由宿主工程决定，可以集中放在ISR源文件，也可以把关键ISR放在`main.c`；参考platform只声明外部ISR并绑定向量，不强制提供ISR实现。
- 应用、协议和服务层不得直接调用DriverLib。
- DriverLib不能表达所需硬件行为时，才允许在目标实现中使用`HWREG/HWREGH`，并在代码旁说明原因。
- 注释说明设计意图、硬件约束、时序、数据所有权和失败后置条件，不逐行复述代码。

## 5. Platform规则

- 每块参考板使用一套通用platform组合入口，不按单个HAL模块建立彼此独立的platform；内部可以按职责拆分源文件。
- `platform.h`只暴露应用或服务真正需要的板级能力；`platform_config.h`集中外设实例、GPIO、IRQ、ACK组、时钟源、XBAR等资源连接宏，并声明用户配置实例；`platform_config.c`定义具有静态存储期的用户可修改HAL配置实例。
- HAL配置实例不得定义为初始化函数的局部变量；HAL运行对象和platform内部状态保持在platform实现内，除非存在明确的跨模块所有权需求。
- platform可以提供清除外设标志和PIE ACK的窄接口供外部ISR调用，但不得在其中实现应用tick、控制算法或产品状态更新。
- platform持有HAL对象并在首次初始化前写入外设实例标识。
- 时钟树、pinmux、IRQ路由、PIE ACK、DMA/XBAR和外部器件控制属于platform。
- platform可以为固定目标保存HAL配置，但不得成为每个运行接口的机械转发层。
- 实时路径在初始化后可以直接使用platform提供的HAL handle。
- 参考platform必须标明硬件假设、调用前置条件和需要由实际项目修改的配置。

## 6. 验证记录

验证记录至少包含：

- 宿主仓库和提交号；
- MCU、开发板和相关外设连接；
- DriverLib与编译器版本；
- 测试日期和测试步骤；
- 编译、下载和运行结果；
- 证据路径或公开链接；
- 未覆盖的负向、压力、异常和回绕场景。

严格区分代码审查结论、用户确认结果和截图直接证据。不得虚构构建尺寸、告警数量、精度、丢失率或截图未显示的测试结果。

## 7. 提交前检查

- 运行`python tools/check_repository.py`；
- 运行`git diff --check`；
- 确认所有自建`.h/.c`文件包含统一文件头，且`File`字段与实际文件名一致；
- 确认没有DriverLib副本、静态库、构建产物或敏感信息；
- 检查公共API、README、Roadmap、验证状态和CHANGELOG是否一致；
- 保留用户已有修改，不使用`git reset --hard`或其他破坏性Git操作。
