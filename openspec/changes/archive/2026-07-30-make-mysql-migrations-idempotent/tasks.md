## 1. MySQL 迁移

- [x] 1.1 将 `009_operation_audit_integrity_schema.sql` 改为列和索引按需创建。
- [x] 1.2 修复迁移登记说明中文乱码。

## 2. 自动预检

- [x] 2.1 在部署预检中检查 MySQL 列/索引迁移是否具备幂等保护。
- [x] 2.2 运行 OpenSpec、后端 CTest 和提交前检查。
- [x] 2.3 提交并推送 GitHub，确认 GitHub Actions 结果。