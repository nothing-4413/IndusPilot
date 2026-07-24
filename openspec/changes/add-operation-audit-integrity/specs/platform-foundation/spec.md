## ADDED Requirements

### Requirement: 操作审计哈希链

系统 SHALL 为新写入的操作审计事件生成基于上一条审计事件哈希的 SHA-256 哈希链。

#### Scenario: 记录首条审计事件

- **GIVEN** 操作审计仓储中没有历史事件
- **WHEN** 后端记录一条新的操作审计事件
- **THEN** 该事件的 `previousHash` SHALL be `genesis`
- **AND** 该事件的 `eventHash` SHALL be non-empty

#### Scenario: 记录后续审计事件

- **GIVEN** 操作审计仓储中已经存在带 `eventHash` 的最新事件
- **WHEN** 后端记录下一条操作审计事件
- **THEN** 新事件的 `previousHash` SHALL equal 最新历史事件的 `eventHash`
- **AND** 新事件的 `eventHash` SHALL be generated from canonical audit payload and `previousHash`

### Requirement: 操作审计完整性校验接口

系统 SHALL 提供受 `audit:read` 权限保护的审计完整性校验接口。

#### Scenario: 管理员校验审计链

- **GIVEN** 管理员已登录并具备 `audit:read` 权限
- **WHEN** 管理员调用 `GET /api/v1/audit/integrity`
- **THEN** 后端 SHALL return 校验结果、记录总数、断点事件编号和最新哈希

#### Scenario: 无审计读取权限用户被拒绝

- **GIVEN** 操作员已登录但不具备 `audit:read` 权限
- **WHEN** 操作员调用 `GET /api/v1/audit/integrity`
- **THEN** 后端 SHALL 返回 HTTP 403

#### Scenario: 审计事件被篡改

- **GIVEN** 已存在一组审计事件
- **WHEN** 任意事件内容或哈希链字段被修改导致重算结果不一致
- **THEN** 完整性校验结果 SHALL mark `verified` as false
- **AND** 响应 SHALL include first broken event id

### Requirement: Qt 客户端审计完整性展示

Qt 客户端 SHALL 在操作审计页面展示审计哈希链完整性状态。

#### Scenario: 刷新操作审计时展示完整性状态

- **GIVEN** 用户打开“操作审计”页面并已连接后端
- **WHEN** 客户端刷新审计列表
- **THEN** 客户端 SHALL call `GET /api/v1/audit/integrity`
- **AND** 页面 SHALL 显示中文完整性状态

#### Scenario: 用户手动校验审计链

- **GIVEN** 用户位于“操作审计”页面
- **WHEN** 用户点击“校验”按钮
- **THEN** 客户端 SHALL refresh the integrity status without exporting audit data