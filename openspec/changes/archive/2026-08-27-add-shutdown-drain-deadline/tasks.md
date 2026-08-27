# 任务清单

## 1. OpenSpec
- [x] 1.1 编写 shutdown drain deadline proposal、design、spec 和 tasks

## 2. 配置与生命周期
- [x] 2.1 增加 shutdown drain timeout 配置、环境变量和严格校验
- [x] 2.2 增加带 timeout 的请求排空等待 API
- [x] 2.3 让 shutdown coordinator 超时后记录剩余请求并退出

## 3. 验证与交付
- [x] 3.1 增加配置和生命周期超时测试
- [x] 3.2 更新生产停机文档和 HTTP shutdown smoke
- [x] 3.3 运行质量门禁、CTest、HTTP smoke，同步主规格并推送 GitHub
