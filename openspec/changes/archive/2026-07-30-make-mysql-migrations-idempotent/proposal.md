# Change: 增强 MySQL 迁移幂等性

## Why

生产环境和演示环境经常会重复执行数据库初始化或增量迁移脚本。`009_operation_audit_integrity_schema.sql` 直接追加审计哈希列和索引，重复运行会因为列或索引已存在而失败，影响部署恢复、CI 预检和现场升级可信度。

## What Changes

- 将审计完整性迁移改为先检查 `INFORMATION_SCHEMA`，仅在缺失时追加列或索引。
- 修复 `009` 迁移登记说明中的中文乱码。
- 部署预检新增 MySQL 列/索引迁移幂等性检查，防止后续迁移重新引入裸 `ALTER TABLE ... ADD COLUMN/INDEX` 风险。

## Non-Goals

- 不改变已有审计表字段语义或哈希链算法。
- 不引入数据库迁移框架或版本回滚机制。
- 不要求预检连接真实 MySQL 实例执行迁移。