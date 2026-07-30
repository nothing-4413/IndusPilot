## ADDED Requirements

### Requirement: 工业闭环演示脚本
系统 SHALL 提供可复现的工业告警到维护工单闭环演示脚本，用于展示核心模块协同能力。

#### Scenario: 运行端到端演示

- **GIVEN** Drogon HTTP 后端已启动
- **AND** 默认演示账号可登录
- **WHEN** 用户运行闭环演示脚本
- **THEN** 脚本 SHALL 创建演示资产和运行状态
- **AND** 脚本 SHALL 创建关键告警并派发通知
- **AND** 脚本 SHALL 调用 AI 诊断接口
- **AND** 脚本 SHALL 创建并关闭维护工单
- **AND** 脚本 SHALL 关闭告警并导出审计 CSV

#### Scenario: 演示输出可追溯

- **GIVEN** 演示脚本执行成功
- **WHEN** 用户查看输出目录
- **THEN** 输出 SHALL 包含本次运行的 summary JSON
- **AND** 输出 SHALL 包含关键业务步骤响应文件
- **AND** 输出 SHALL 包含审计 CSV 文件