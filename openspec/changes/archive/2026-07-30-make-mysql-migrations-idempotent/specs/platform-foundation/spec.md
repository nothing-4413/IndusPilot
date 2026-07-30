## ADDED Requirements

### Requirement: MySQL 增量迁移可重复执行
系统 SHALL 保证已纳入部署预检的 MySQL 增量迁移在目标列或索引已存在时不会因重复创建而失败。

#### Scenario: 重复执行审计完整性迁移

- **GIVEN** `operation_audit_events` 已存在 `previous_hash`、`event_hash` 和 `idx_operation_audit_event_hash`
- **WHEN** 再次执行 `009_operation_audit_integrity_schema.sql`
- **THEN** 迁移 SHALL 跳过已有列和索引，并继续登记 `009_operation_audit_integrity_schema`

#### Scenario: 预检发现非幂等列或索引变更

- **GIVEN** MySQL 迁移脚本通过 `ALTER TABLE` 添加列或索引
- **WHEN** 脚本没有使用 `INFORMATION_SCHEMA` 或 `IF NOT EXISTS` 判断目标对象是否已存在
- **THEN** 部署预检 SHALL 报告失败并指出对应脚本