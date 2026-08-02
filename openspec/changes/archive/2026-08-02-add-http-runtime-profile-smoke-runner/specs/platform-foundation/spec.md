## ADDED Requirements

### Requirement: HTTP 真实运行时 profile 验收脚本
系统 SHALL 提供一个可复现脚本，用于在真实依赖环境中执行 MySQL 仓储和 Redis session 的 HTTP smoke。

#### Scenario: 从部署环境读取连接参数

- **GIVEN** `deployment/.env` 已存在并包含 MySQL、Redis、MongoDB 依赖配置
- **WHEN** 开发者运行 HTTP runtime profile 验收脚本
- **THEN** 脚本 SHALL 从 `.env` 读取端口、账号和密钥
- **AND** 脚本 SHALL 组合 HTTP smoke 所需的 MySQL、Redis 和 MongoDB 连接参数

#### Scenario: 拒绝示例密钥

- **GIVEN** `deployment/.env` 仍包含 `change-me-*` 示例密钥
- **WHEN** 开发者运行 HTTP runtime profile 验收脚本
- **THEN** 脚本 SHALL 失败并提示替换密钥
- **AND** 脚本 SHALL NOT 启动后端 HTTP smoke

#### Scenario: 执行真实运行时 HTTP smoke

- **GIVEN** 后端 `dev-http` 构建产物存在
- **AND** MySQL、Redis、MongoDB 依赖已经启动并通过 dependency smoke
- **WHEN** 脚本执行 HTTP smoke
- **THEN** 脚本 SHALL 使用 `repository_store=mysql` 和 `session_store=redis`
- **AND** 脚本 SHALL 复用 `backend/tests/http_integration_smoke.ps1` 覆盖核心 HTTP 业务链路

#### Scenario: 可选编排依赖生命周期

- **WHEN** 开发者传入启动或清理开关
- **THEN** 脚本 SHALL 使用 `deployment/docker-compose.yml` 启动或清理真实依赖