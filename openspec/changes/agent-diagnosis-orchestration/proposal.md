# Change: Agent 诊断编排能力

## Why

当前 AI 模块已经支持降级建议和交互审计，但还没有体现 agent 型系统的关键能力：Provider 抽象、上下文编排、结构化诊断输出和可审计执行路径。为了让项目更适合展示 agent 工程能力，需要把 AI 辅助从单条文本建议升级为可配置、可降级、可审计的诊断编排能力。

## What Changes

- 增加 `AiProvider` 抽象，支持 `disabled` 和 `http` provider 配置边界。
- 增加 `DiagnosisAgent` 编排器，将告警、运行状态、工单历史和人工描述组织为诊断上下文。
- 增加结构化诊断结果：摘要、可能原因、建议动作、风险等级和人工复核标记。
- 增加 `POST /api/v1/ai/diagnose`，复用 `ai:use` 权限和 AI 交互审计仓储。
- 更新配置、测试和文档，明确无外部模型时的可用降级路径。

## Non-Goals

- 不在本次绑定 OpenAI 或特定商业模型 SDK。
- 不在本次实现复杂 RAG 知识库检索。
- 不在本次实现 Qt 客户端真实调用该接口。