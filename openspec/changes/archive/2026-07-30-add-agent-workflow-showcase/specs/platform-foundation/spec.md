## ADDED Requirements

### Requirement: Agent 工作流展示证据
系统 SHALL 提供可生成的 Agent 工作流证据包，用于展示需求、规格、实现、验证、提交和 CI 的交付闭环。

#### Scenario: 生成 Agent 证据包
- **GIVEN** 开发者位于仓库工作区
- **WHEN** 运行 Agent 工作流证据脚本
- **THEN** 脚本 SHALL 生成 JSON 结构化证据和 Markdown 展示摘要
- **AND** 证据 SHALL 包含最近提交、OpenSpec 归档、CI job 和工业闭环摘要的可用信息

#### Scenario: 查看 Agent 交付说明
- **GIVEN** 访问项目文档
- **WHEN** 开发者打开 Agent 工作流展示文档
- **THEN** 文档 SHALL 描述从 OpenSpec 到 GitHub Actions 的闭环链路
- **AND** 文档 SHALL 说明工业 AI 诊断中的 agent 编排边界