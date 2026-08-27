# 设计：AI HTTP provider 运行边界

## 调用链

```text
HTTP diagnose route
        │
        ▼
    AiService
        │  local orchestration: risk/causes/actions
        ├──────────────────────────────┐
        ▼                              │
   AiProvider::complete               │
        │                              │
        ▼                              │
  HTTP provider request               │
        │                              │
        └── available/content ─────────┘
```

HTTP provider 只负责把操作、提示词和受限上下文传输到配置的 endpoint，并返回可用文本；`AiService` 继续负责非权威结构化诊断编排。调用失败、超时、非 2xx、空文本或要求 JSON 时收到非 JSON，均返回不可用 provider 结果，但不阻断诊断响应和 AI 交互审计。

## 测试边界

fake provider 是独立的 PowerShell `HttpListener` 进程，只用于集成测试。它将收到的 method、path、headers 和 JSON body 写入临时日志，并按测试模式返回成功、失败、非 JSON 或延迟响应。测试每个场景都使用独立后端进程和随机本地端口，避免共享状态造成误判。

## 安全与稳定性

- smoke 只使用测试 API key，且仅通过环境变量注入。
- provider 请求最多发送 `ai.max_context_items` 条上下文。
- 测试确认自定义鉴权头和 `X-IndusPilot-Ai-Operation`，但不把密钥写入 AI 交互审计断言。
- fake provider 和后端都在 `finally` 中终止并清理临时文件。
