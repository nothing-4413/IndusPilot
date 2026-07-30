## ADDED Requirements

### Requirement: HTTP 响应回传请求追踪编号
系统 SHALL 为所有 Drogon HTTP 响应回传可用于日志和审计关联的追踪编号。

#### Scenario: 客户端传入 X-Request-Id

- **GIVEN** HTTP 请求包含 `X-Request-Id`
- **WHEN** 后端返回响应
- **THEN** 响应头 SHALL 包含同值的 `X-Trace-Id` 和 `X-Request-Id`

#### Scenario: 客户端传入 X-Trace-Id

- **GIVEN** HTTP 请求包含 `X-Trace-Id`
- **WHEN** 后端写入结构化请求日志或操作审计
- **THEN** 日志与审计 SHALL 使用该追踪编号

#### Scenario: 客户端未传入追踪头

- **GIVEN** HTTP 请求未包含 `X-Trace-Id` 或 `X-Request-Id`
- **WHEN** 后端处理请求
- **THEN** 系统 SHALL 生成 `trace-<timestamp>-<sequence>` 格式追踪编号并在响应头回传