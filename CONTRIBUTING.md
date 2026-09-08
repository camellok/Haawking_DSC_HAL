# 贡献指南

感谢为Haawking DSC HAL提交改进。主动提交到本仓库的代码和文档按Apache-2.0许可证提供；提交贡献表示贡献者有权提供该内容。

## 开发步骤

1. 建立描述需求和验收方法的Issue；
2. 从`main`创建短期功能分支；
3. 保持HAL、platform和应用职责边界；
4. 更新必要的API文档、验证记录和CHANGELOG；
5. 运行`python tools/check_repository.py`和`git diff --check`；
6. 在适用宿主工程中完成编译和硬件测试；
7. 提交Pull Request并填写兼容性和验证结果。

公共API的名称、参数、状态语义或生命周期发生变化时，PR必须标注兼容性影响。没有硬件证据的实现应明确保持“待验证”状态。
