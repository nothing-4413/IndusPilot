# Change: 增加 HTTP 请求追踪可观测性

## Why

工业运维系统需要把一次 API 调用、结构化日志和操作审计串起来。当前 HTTP 层已有 `traceId` 日志和审计字段，但响应没有统一回传追踪头，也没有兼容通用的 `X-Request-Id`。这会让调用方和排障人员难以从前端请求定位到后端日志与审计记录。

## What Changes

- HTTP 层兼容 `X-Trace-Id` 与 `X-Request-Id`，优先使用 `X-Trace-Id`。
- 未传入追踪头时生成 `trace-<timestamp>-<sequence>`。
- 所有 Drogon 响应统一回传 `X-Trace-Id` 与 `X-Request-Id`。
- HTTP smoke 测试覆盖 `X-Request-Id` 到响应追踪头的传播。
- 开发文档补充请求追踪约定。

## Non-Goals

- 不引入 OpenTelemetry、Prometheus 或外部日志系统。
- 不改变现有 API JSON 响应结构。
- 不改造所有业务日志字段。