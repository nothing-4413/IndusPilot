## ADDED Requirements

### Requirement: 会话注销可审计且区分存储故障
系统 SHALL 在成功注销已验证会话时写入不包含 token 的 `auth.logout` 审计事件。当已验证会话无法从会话存储删除时，系统 SHALL 返回服务不可用而非未授权。

#### Scenario: 成功注销会话

- **GIVEN** 请求携带有效 Bearer session
- **WHEN** 客户端调用注销接口
- **THEN** 会话 SHALL 被撤销
- **AND** 系统 SHALL 记录包含操作者、用户资源和 trace id 的 `auth.logout` 审计事件
- **AND** 审计事件 SHALL NOT 包含 session token

#### Scenario: 已验证会话无法撤销

- **GIVEN** 请求携带有效 Bearer session
- **AND** 会话存储在删除时失败
- **WHEN** 客户端调用注销接口
- **THEN** 接口 SHALL 返回 HTTP 503
