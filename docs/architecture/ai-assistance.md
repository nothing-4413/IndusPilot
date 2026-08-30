# AI 辅助诊断边界

AI 模块只提供辅助解释、日志摘要和排查建议，不直接关闭告警、不自动完成工单、不替代人工确认。

## 当前能力

- 告警解释接口：接收告警摘要和上下文
- 排查建议接口：接收关联对象、提示词和上下文列表
- 日志摘要接口：为后续 MongoDB 日志接入预留
- 不可用兜底：AI 未启用时返回清晰状态
- agent 诊断编排：接收结构化上下文，生成风险等级、可能原因、建议动作和人工复核提示
- 交互记录：保存输入、输出和关联业务对象；`repository_store=mysql` 时写入 MySQL 审计表

## 后续接入

后续可以在 Provider 边界内替换适配器，实现远程大模型、本地模型或企业知识库 RAG。无论 provider 是否可用，AI 结果都应保持非阻塞、可审计、可降级。

## 外部调用安全边界

HTTP provider 的所有尝试共享 `ai.timeoutMs` 总预算，`ai.max_retries` 只对 transport、408、429 和 5xx 生效；`ai.max_response_bytes` 在解析前拒绝超限响应。prompt/context 和保存到 AI 交互仓储的输入摘要会对常见凭据键值进行 `[REDACTED]` 脱敏。当前 provider 文本仍不是权威指令，风险等级、可能原因、建议动作和人工复核标记由本地诊断编排器生成。

## HTTP provider 请求契约


启用 Drogon 构建并设置 `ai.enabled=true`、`ai.provider=http` 后，AI provider 会向 `ai.endpoint` 发起 POST 请求。请求体包含 `operation`、`prompt` 和受 `ai.maxContextItems` 限制的 `contextItems`。`ai.timeoutMs` 控制总请求超时，`ai.max_retries` 只对可重试失败生效；`ai.storeInteractionRecords=false` 时不写入 AI 交互审计仓储。响应可返回 `content`、`summary`、`text`、`output_text`，也兼容 OpenAI `choices[].message.content` 和 Responses `output[].content[].text`。HTTP 调用失败、超时、响应过大或状态码非 2xx 时，系统保留本地规则诊断并记录降级原因。
