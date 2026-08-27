# 设计：AI provider 外部调用边界

## 调用预算

```text
diagnosis request
      │
      ▼
 sanitize prompt/context
      │
      ▼
 absolute deadline = now + timeout_ms
      │
      ├─ attempt 1 ── transient failure ──┐
      ├─ attempt 2 ── transient failure ──┤ max_retries
      └─ 2xx response ── size check ──────┘
                              │
                         parse or fallback
```

`max_retries` 表示额外尝试次数，默认 0 保持现有行为。只有 transport failure、408、429 和 5xx 可重试；每次调用使用 absolute deadline 剩余时间，整个 provider 操作不会超过 `timeout_ms`（加上极小的调度误差）。

## 脱敏策略

脱敏器只处理明确跟随 `:` 或 `=` 的常见敏感键值，保留键名和分隔符并将值替换为 `[REDACTED]`。它应用于发往外部 provider 的 prompt/context，也应用于保存到 AI interaction repository 的输入摘要；鉴权头只使用配置值发送，不写入日志或审计。

## 响应边界

先读取 response body 的字节数并与 `max_response_bytes` 比较，超限返回不可用结果。合规响应继续沿用已有 JSON 文本字段提取和 `require_structured_response` 规则。
