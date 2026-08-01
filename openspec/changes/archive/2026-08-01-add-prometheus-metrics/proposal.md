# Change: 增加 Prometheus 可观测性指标

## Why

工业运维支持系统需要能够被持续观测，当前健康检查和日志已经存在，但缺少可被 Prometheus/Grafana 采集的指标端点。为了提升生产级可信度，需要为 HTTP 请求、错误、AI 调用和关键闭环动作提供低基数指标。

## What Changes

- 增加 `MetricsRegistry`，聚合 HTTP 请求、错误、AI 请求、告警关闭和工单关闭指标。
- Drogon 运行时通过统一 advice 记录请求状态码和耗时。
- 增加 `GET /metrics`，返回 Prometheus 文本格式指标。
- HTTP smoke 测试覆盖 `/metrics` 输出。
- 增加可观测性指标开发文档和 README 入口。

## Non-Goals

- 本次不引入 Prometheus/Grafana 容器编排。
- 本次不做分布式 tracing 后端接入。