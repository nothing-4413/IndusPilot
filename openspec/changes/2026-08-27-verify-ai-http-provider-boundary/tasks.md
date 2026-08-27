# 任务清单

## 1. OpenSpec
- [x] 1.1 编写 AI HTTP provider 边界的 proposal、design、spec 和 tasks

## 2. Provider 集成验证
- [x] 2.1 增加可记录请求并可模拟响应模式的本地 fake provider
- [x] 2.2 增加 HTTP provider smoke，覆盖成功、失败、非 JSON 和超时降级
- [x] 2.3 将 smoke 接入 dev-http CTest，供构建矩阵和 CI 调用

## 3. 文档与交付
- [x] 3.1 修正文档中的 provider 传输边界和结构化结果责任
- [x] 3.2 同步 AI 主规格并运行质量门禁、CTest、HTTP smoke
- [ ] 3.3 归档 change，提交并推送 GitHub
