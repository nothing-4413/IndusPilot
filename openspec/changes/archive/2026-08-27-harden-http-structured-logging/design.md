# 设计：HTTP 结构化日志

## 序列化边界

`writeRequestLog` 负责组装一个 `Json::Value` 对象，并通过 JsonCpp 的 writer 输出单行 JSON。所有动态字段作为 JSON string 写入，不再手工拼接引号或转义符。

```text
request headers/path/session
          │
          ▼
    Json::Value event
          │
          ▼
    JsonCpp compact writer
          │
          ▼
    one valid JSON line on stdout
```

## 兼容性

- 保留 `event`、`traceId`、`method`、`path` 和 `user` 字段。
- 仍由现有 HTTP post/pre handling advice 调用，不改变 trace header 或 metrics 行为。
- 测试通过捕获 stdout 并使用 JsonCpp 解析，证明日志可被机器消费。
