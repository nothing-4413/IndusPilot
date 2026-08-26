# 任务清单

## 1. OpenSpec
- [x] 1.1 编写生产 readiness/startup proposal、design、spec 和 tasks
- [x] 1.2 完成等价仓库结构校验

## 2. 配置与依赖需求
- [x] 2.1 增加 readiness 探测超时、缓存和 AI required 配置及环境变量覆盖
- [x] 2.2 增加静态配置校验，错误配置阻止启动
- [ ] 2.3 让 DataConnectors 只探测实际参与运行时的依赖

## 3. 启动状态与 HTTP 检查
- [ ] 3.1 增加 liveness/readiness/startup 状态模型
- [ ] 3.2 实现 `/health/live`、`/health/ready`、`/health/startup` 和兼容 `/health`
- [ ] 3.3 实现依赖探测超时、缓存和恢复后的重新评估

## 4. 部署、测试与文档
- [ ] 4.1 扩展 HTTP smoke 验证三类 health endpoint 和 503 语义
- [ ] 4.2 更新 Docker healthcheck、部署配置和运维文档
- [ ] 4.3 运行质量门禁、CTest、HTTP smoke 并按阶段提交推送
