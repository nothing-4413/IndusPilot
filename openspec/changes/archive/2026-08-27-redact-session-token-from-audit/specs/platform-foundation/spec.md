## ADDED Requirements

### Requirement: 登录审计不得保存 session token
系统 SHALL 在登录成功审计事件中使用稳定用户标识，不得将可复用的 session token 写入审计事件、审计查询结果或审计导出。

#### Scenario: 登录成功写入审计

- **WHEN** 用户凭有效凭据登录成功
- **THEN** 审计事件 SHALL 记录用户标识和 `auth.login` 动作
- **AND** 审计事件的资源标识 SHALL NOT 等于返回给客户端的 session token

#### Scenario: 查询或导出登录审计

- **WHEN** 具备审计权限的用户查询或导出登录审计事件
- **THEN** 结果 SHALL NOT 包含登录 session token
- **AND** 审计动作、追踪编号和完整性字段 SHALL 保持可用
