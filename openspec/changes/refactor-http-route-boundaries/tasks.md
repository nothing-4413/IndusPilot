# 任务清单

## 1. OpenSpec
- [x] 1.1 编写 HTTP 模块边界 proposal、design、spec 和 tasks
- [x] 1.2 完成严格结构校验或等价仓库验证

## 2. HTTP 公共边界
- [x] 2.1 提取 HttpServerContext 和 composition root
- [x] 2.2 提取公共响应、认证授权、trace、metrics 和分页 helper
- [x] 2.3 保持现有 `/health`、`/metrics` 和错误响应兼容

## 3. 业务路由拆分
- [x] 3.1 拆分认证、资产和运行监控路由
- [ ] 3.2 拆分告警和维护工单路由
- [ ] 3.3 拆分操作审计和 AI 路由
- [ ] 3.4 让主 HTTP 启动文件只负责运行时启动和 registrar 编排
- [ ] 3.5 按业务上下文迁移对应 DTO 和序列化 helper

## 4. 验证与交付
- [ ] 4.1 运行质量门禁、PowerShell 语法检查和 CTest
- [ ] 4.2 运行 HTTP integration smoke 并确认 API 行为兼容
- [ ] 4.3 按可验证部分提交并推送 GitHub
