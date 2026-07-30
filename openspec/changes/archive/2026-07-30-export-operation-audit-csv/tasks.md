## 1. OpenSpec

- [x] 1.1 编写操作审计 CSV 导出变更说明和规格增量。

## 2. 后端与权限

- [x] 2.1 新增 `audit:export` 权限并默认授予管理员。
- [x] 2.2 新增操作审计 CSV 导出接口。
- [x] 2.3 导出操作写入 `operation-audit.export` 审计事件。
- [x] 2.4 补充 HTTP 冒烟测试覆盖权限和 CSV 内容。

## 3. Qt 客户端

- [x] 3.1 新增操作审计 CSV 下载 API。
- [x] 3.2 在“操作审计”页面增加导出按钮和保存文件流程。

## 4. 文档、验证与提交

- [x] 4.1 更新 README、HTTP 服务、本地启动和生产就绪说明。
- [x] 4.2 运行 OpenSpec、后端、HTTP 和 Qt 构建测试。
- [x] 4.3 提交并推送 GitHub，确认 GitHub Actions 结果。