# 任务清单

## 1. OpenSpec
- [x] 1.1 编写配置错误与 readiness 并发的 proposal、design、spec 和 tasks
- [x] 1.2 核对当前配置、探测、应用状态和 HTTP 测试边界

## 2. 严格配置加载
- [ ] 2.1 为 AppConfig 增加加载错误承载，并让 validateConfig 统一报告文件/解析错误
- [ ] 2.2 严格解析文件和环境变量的整数、布尔值、section、key、层级与损坏行
- [ ] 2.3 增加缺失文件、非法类型、未知字段和启动失败测试

## 3. readiness 探测并发与 deadline
- [ ] 3.1 将探测从 Application 状态锁移出，增加 single-flight 合并和生命周期安全
- [ ] 3.2 并行执行启用的依赖探测，并让 DNS/连接遵守统一 absolute deadline
- [ ] 3.3 增加探测 metadata、失败/恢复边沿计数和状态序列化

## 4. 集成验证与交付
- [ ] 4.1 扩展基础测试和 HTTP smoke 覆盖新契约
- [ ] 4.2 更新生产 readiness 文档和配置说明
- [ ] 4.3 运行质量门禁、CTest、HTTP smoke，并按阶段提交推送
