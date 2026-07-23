# 任务清单

## 1. OpenSpec
- [x] 1.1 编写 agent 诊断编排提案和规格增量
- [x] 1.2 OpenSpec 变更严格校验通过

## 2. Provider 与编排模型
- [ ] 2.1 扩展 AI 配置，支持 provider 选择和环境变量覆盖
- [ ] 2.2 增加 AI Provider 抽象和 disabled/http provider 边界
- [ ] 2.3 增加诊断请求、上下文和结构化结果模型
- [ ] 2.4 实现 `DiagnosisAgent` 编排与降级诊断

## 3. HTTP 与审计
- [ ] 3.1 增加 `POST /api/v1/ai/diagnose`
- [ ] 3.2 诊断请求写入 AI 交互审计仓储
- [ ] 3.3 状态接口返回 provider 配置边界

## 4. 测试与文档
- [ ] 4.1 补充配置和诊断编排单测
- [ ] 4.2 扩展 HTTP 冒烟测试覆盖诊断接口
- [ ] 4.3 更新配置示例和开发/部署文档
- [ ] 4.4 默认构建、`dev-http` 测试和 OpenSpec 严格校验通过

## 5. 提交
- [ ] 5.1 按模块提交 Git 记录