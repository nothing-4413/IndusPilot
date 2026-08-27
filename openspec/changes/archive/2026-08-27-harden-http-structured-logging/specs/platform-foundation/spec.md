## ADDED Requirements

### Requirement: HTTP 请求日志必须是合法结构化 JSON
系统 SHALL 使用结构化 JSON 序列化 HTTP 请求日志中的动态字段，并 SHALL 防止请求输入破坏日志记录格式。

#### Scenario: 请求字段包含 JSON 特殊字符

- **GIVEN** 请求追踪编号、路径或用户名包含引号、反斜杠或换行
- **WHEN** HTTP 请求日志写入 stdout
- **THEN** 日志行 SHALL 仍是可解析的 JSON
- **AND** 动态字段 SHALL 保留原始字符语义

#### Scenario: 日志字段保持兼容

- **WHEN** HTTP 请求日志写入
- **THEN** 日志 SHALL 继续包含 `event`、`traceId`、`method`、`path` 和 `user` 字段
- **AND** SHALL NOT 输出认证密钥或 session token
