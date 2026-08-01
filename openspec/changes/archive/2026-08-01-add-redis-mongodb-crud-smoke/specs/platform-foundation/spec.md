## ADDED Requirements

### Requirement: Redis/MongoDB 真实 CRUD 集成测试
系统 SHALL 在 dependency-services CI 中对 Redis 和 MongoDB 执行真实读写冒烟测试，并在失败时阻断 CI。

#### Scenario: Redis 真实读写验证

- **GIVEN** Redis 容器已启动并启用密码认证
- **WHEN** dependency smoke 执行 Redis 验证
- **THEN** 测试 SHALL 使用认证连接写入并读取 key/value 数据
- **AND** 测试 SHALL 验证 TTL、counter 和 hash 字段读写可用

#### Scenario: MongoDB 文档 CRUD 验证

- **GIVEN** MongoDB 容器已启动并完成初始化
- **WHEN** dependency smoke 执行 MongoDB 验证
- **THEN** 测试 SHALL 确认 `operation_logs`、`ai_interactions` 和 `diagnostic_documents` 集合存在
- **AND** 测试 SHALL 在这些集合中执行可重复的文档 upsert/read 断言

#### Scenario: MongoDB 初始化索引验证

- **GIVEN** MongoDB 初始化脚本已经执行
- **WHEN** MongoDB CRUD smoke 运行
- **THEN** 测试 SHALL 验证核心查询索引和诊断文档文本索引存在
- **AND** 测试 SHALL 在成功时输出 `mongodb_real_crud_smoke_passed`