# Change: MongoDB AI 交互仓储

## ADDED Requirements

### Requirement: AI 交互记录可选择 MongoDB 持久化

系统 SHALL 支持独立选择 AI 交互记录的 `memory`、`mysql` 或 `mongodb` 仓储，默认行为保持不变。

#### Scenario: 选择 MongoDB 仓储
- **GIVEN** `storage.ai_interaction_store=mongodb` 且 MongoDB URI 有效
- **WHEN** AI 诊断或建议产生交互记录
- **THEN** 记录 SHALL 以 upsert 形式写入 `ai_interactions` 集合
- **AND** 关联对象查询 SHALL 按 `relatedType`、`relatedId` 返回记录

#### Scenario: MongoDB 暂时不可用
- **GIVEN** AI 交互仓储选择为 MongoDB 且 MongoDB 不可用
- **WHEN** 系统读取或保存 AI 交互记录
- **THEN** 系统 SHALL 返回可诊断的依赖错误
- **AND** 不得将失败记录报告为已持久化成功

#### Scenario: 保持既有仓储兼容
- **GIVEN** 未配置 `storage.ai_interaction_store` 或其值为 `memory/mysql`
- **WHEN** 后端启动并处理 AI 交互
- **THEN** 系统 SHALL 继续使用对应既有仓储实现

### Requirement: MongoDB 业务仓储受运行时依赖治理保护

系统 SHALL 仅在选择 MongoDB AI 交互仓储时将 MongoDB 标记为 required，并在未启用 MongoDB driver 的构建中拒绝该配置。

#### Scenario: MongoDB 仓储进入 readiness
- **GIVEN** `storage.ai_interaction_store=mongodb`
- **WHEN** 操作员请求 readiness
- **THEN** MongoDB 依赖 SHALL 被标记为 required，并反映连接可用性

#### Scenario: 未安装 MongoDB adapter
- **GIVEN** 当前后端构建未包含 MongoDB driver
- **WHEN** 配置选择 `storage.ai_interaction_store=mongodb`
- **THEN** 配置校验 SHALL 在 listener 启动前失败
