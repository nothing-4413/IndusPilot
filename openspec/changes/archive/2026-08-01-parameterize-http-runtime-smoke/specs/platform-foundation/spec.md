## ADDED Requirements

### Requirement: HTTP smoke 支持真实运行时 profile
系统 SHALL 允许同一套 HTTP smoke 在默认内存模式和真实依赖运行时模式之间切换。

#### Scenario: 默认 CTest 使用内存运行时

- **GIVEN** 开发者运行 `ctest --preset dev-http`
- **WHEN** CTest 执行 `induspilot-http-integration-smoke`
- **THEN** HTTP smoke SHALL 使用 `repository_store=memory` 和 `session_store=memory`
- **AND** 测试 SHALL 不要求本机已启动 MySQL、Redis 或 MongoDB

#### Scenario: 手动验证 MySQL 仓储运行时

- **GIVEN** MySQL 依赖已经初始化并包含 IndusPilot schema 与种子账号
- **WHEN** 开发者运行 HTTP smoke 并传入 `-RepositoryStore mysql` 和 MySQL 连接参数
- **THEN** 后端 SHALL 使用 MySQL 仓储执行同一套登录、资产、告警、工单、AI 审计和操作审计接口验证

#### Scenario: 手动验证 Redis-backed session

- **GIVEN** Redis 依赖已经启动并配置连接 URI
- **WHEN** 开发者运行 HTTP smoke 并传入 `-SessionStore redis` 和 Redis URI
- **THEN** 后端 SHALL 使用 Redis-backed session 完成登录、会话验证和受保护路由访问

#### Scenario: HTTP smoke 不污染调用环境

- **WHEN** HTTP smoke 设置仓储、会话或依赖连接环境变量
- **THEN** 脚本结束时 SHALL 恢复调用前的环境变量值