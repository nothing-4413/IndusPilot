# 任务清单

## 1. OpenSpec
- [x] 1.1 编写优雅停机、请求排空、信号和 listener 失败的 proposal、design、spec 和 tasks
- [x] 1.2 核对 Drogon lifecycle/signal API 与现有 health route 边界

## 2. Application 生命周期
- [x] 2.1 增加 running/draining/stopped 状态和幂等生命周期操作
- [x] 2.2 让 readiness/liveness/startup 序列化正确反映 draining 与 stopped
- [x] 2.3 增加 Application 生命周期单元测试

## 3. HTTP 排空与 runtime
- [x] 3.1 增加统一 HTTP 请求 gate 和在途请求计数
- [x] 3.2 接入 TERM/INT shutdown coordinator，停止接收新请求并等待排空
- [x] 3.3 处理 listener 绑定失败并定义非零退出码
- [x] 3.4 增加 HTTP shutdown smoke，验证 shutdown coordinator 能处理 SIGINT 并正常退出；ready/live/新业务请求语义由生命周期与 gate 测试覆盖

## 4. 交付
- [x] 4.1 更新部署、运维和生产 readiness 文档
- [ ] 4.2 运行 CTest、HTTP smoke、质量门禁和 `git diff --check`
- [ ] 4.3 每个实现阶段提交并推送 GitHub，完成后归档 OpenSpec change
