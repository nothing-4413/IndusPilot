## ADDED Requirements

### Requirement: CI 启动真实依赖执行冒烟测试
系统 SHALL 在 CI 中启动项目 compose 定义的 MySQL、Redis 和 MongoDB，并执行最小真实依赖冒烟验证。

#### Scenario: 真实依赖编排启动

- **GIVEN** GitHub Actions 运行主分支或拉取请求 CI
- **WHEN** 执行依赖集成测试 job
- **THEN** CI SHALL 使用 `deployment/docker-compose.yml` 和 CI 专用 `.env` 启动 MySQL、Redis、MongoDB
- **AND** job 完成后 SHALL 清理 compose 容器和卷

#### Scenario: MySQL 迁移可在真实数据库重复执行

- **GIVEN** MySQL 容器已启动并完成初始化
- **WHEN** 依赖冒烟测试重复执行 `database/mysql/*.sql`
- **THEN** 迁移 SHALL 成功完成并能查询到 `schema_migrations` 与审计哈希列

#### Scenario: Redis 和 MongoDB 最小可用验证

- **GIVEN** Redis 和 MongoDB 容器已启动
- **WHEN** 依赖冒烟测试运行
- **THEN** Redis SHALL 通过密码执行 `PING`
- **AND** MongoDB SHALL 加载初始化脚本并通过 `ping` 命令