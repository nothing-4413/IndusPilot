## ADDED Requirements

### Requirement: 登录失败锁定与限流
系统 SHALL 在短时间连续登录失败达到阈值后临时锁定该登录主体，并提供可配置的失败窗口、失败阈值和锁定时长。

#### Scenario: 连续失败触发锁定

- **GIVEN** 登录锁定策略已启用
- **WHEN** 同一用户名在失败窗口内连续失败达到阈值
- **THEN** 后续登录 SHALL 返回 `AUTHENTICATION_LOCKED`
- **AND** HTTP 响应 SHALL 使用 `429 Too Many Requests`
- **AND** HTTP 响应 SHALL 包含 `Retry-After`

#### Scenario: 失败登录写入审计

- **GIVEN** 登录请求认证失败或命中锁定
- **WHEN** HTTP 服务处理该请求
- **THEN** 系统 SHALL 写入 `auth.login.failed` 或 `auth.login.locked` 操作审计事件
- **AND** 审计事件 SHALL 包含当前请求追踪编号

#### Scenario: 成功登录清除失败状态

- **GIVEN** 用户名存在历史失败登录记录但尚未锁定
- **WHEN** 用户使用正确凭据登录成功
- **THEN** 系统 SHALL 清除该用户名的失败登录状态