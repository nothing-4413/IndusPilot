## ADDED Requirements

### Requirement: 客户端追踪编号必须满足安全格式
系统 SHALL 仅复用符合安全格式的客户端 `X-Trace-Id` 或 `X-Request-Id` 值。安全格式为 1 至 128 个可见 ASCII 非空白字符；其他输入 SHALL 视为无效并由系统生成新的追踪编号。

#### Scenario: 有效客户端追踪编号被回传

- **GIVEN** HTTP 请求包含有效的 `X-Trace-Id`
- **WHEN** 后端返回响应或写入请求日志、操作审计
- **THEN** 响应头、日志和审计 SHALL 使用该编号

#### Scenario: 无效客户端追踪编号回退为服务端编号

- **GIVEN** HTTP 请求包含空白、控制字符、非 ASCII 字节或超过 128 字符的 `X-Trace-Id` 或 `X-Request-Id`
- **WHEN** 后端处理请求
- **THEN** 系统 SHALL 生成 `trace-<timestamp>-<sequence>` 格式追踪编号
- **AND** 响应头、日志和审计 SHALL 使用生成的编号

#### Scenario: 无效 X-Trace-Id 不回退到 X-Request-Id

- **GIVEN** HTTP 请求包含无效的 `X-Trace-Id` 和有效的 `X-Request-Id`
- **WHEN** 后端处理请求
- **THEN** 系统 SHALL 生成新的追踪编号而非复用 `X-Request-Id`
