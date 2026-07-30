## 1. 仓储边界

- [x] 1.1 为审计仓储增加 `latest()` 和 `listForIntegrity()`。
- [x] 1.2 MySQL 查询列表保留最近 500 条，完整性校验使用全量正序列表。
- [x] 1.3 内存仓储和服务层追加路径增加互斥。

## 2. 验证与提交

- [x] 2.1 补充基础测试断言查询顺序和完整性校验。
- [x] 2.2 运行 OpenSpec、后端 CTest、提交并推送 GitHub。
- [x] 2.3 确认 GitHub Actions 结果。