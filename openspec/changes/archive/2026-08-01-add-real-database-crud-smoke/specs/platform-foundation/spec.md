## ADDED Requirements

### Requirement: 真实 MySQL CRUD 集成测试
系统 SHALL 在真实 MySQL 依赖容器中执行可重复的业务 CRUD 冒烟测试，并在失败时阻断 CI。

#### Scenario: CI 启动依赖后执行真实 CRUD

- **GIVEN** GitHub Actions dependency-services job 已启动 MySQL 容器并完成初始化
- **WHEN** dependency smoke 执行 MySQL 验证
- **THEN** 测试 SHALL 在 `induspilot` 数据库中运行真实 CRUD SQL
- **AND** SQL SHALL 在成功时输出 `mysql_real_crud_smoke_passed`

#### Scenario: CRUD 覆盖核心业务表

- **GIVEN** MySQL 迁移脚本已经执行
- **WHEN** 真实 CRUD smoke 运行
- **THEN** 测试 SHALL 覆盖默认用户、资产、运行状态、告警规则、告警、通知投递、工单、附件、AI 交互和操作审计事件
- **AND** 测试 SHALL 校验资产、告警和工单之间的关联关系

#### Scenario: CRUD smoke 可重复执行

- **GIVEN** 上一次 dependency smoke 已经写入 `db-smoke-*` 测试数据
- **WHEN** 再次运行 dependency smoke
- **THEN** 测试 SHALL 使用幂等 upsert 或更新语义继续通过
- **AND** 测试 SHALL 校验 `schema_migrations` 包含当前 MySQL 基线版本