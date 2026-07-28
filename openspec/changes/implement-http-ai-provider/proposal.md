# Change: 实现 HTTP AI Provider 推理传输

## Why

AI 模块已经具备诊断编排、交互审计和 provider 边界，但 `http` provider 仍只返回本地规则降级说明，没有向外部模型服务发起请求。同时 `ai.example.yaml` 中的 `timeoutMs`、`maxContextItems` 和 `storeInteractionRecords` 没有进入运行时配置，容易造成参数存在但无效的问题。

## What Changes

- 扩展 `AiConfig`，加载 AI 超时、上下文条数限制和交互记录开关，并支持环境变量覆盖。
- 在启用 Drogon 构建时，`http` provider 向 `ai.endpoint` 发起 JSON POST 请求。
- HTTP provider 支持从常见响应字段和 OpenAI 风格响应中提取文本内容。
- HTTP 调用失败、超时、非 2xx 或 endpoint 无效时保留本地规则降级，并在 provider 输出中记录原因。
- `storeInteractionRecords=false` 时不写入 AI 交互记录。
- 更新示例配置和 AI 架构文档。

## Non-Goals

- 不在本模块绑定具体云厂商鉴权、API key 或模型名称。
- 不引入异步任务队列或流式响应。
- 不替代现有本地规则诊断结果结构。