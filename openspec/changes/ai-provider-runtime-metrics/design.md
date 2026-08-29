# Design: AI provider runtime metrics

`MetricsRegistry` owns an `AiProviderMetricSnapshot` map keyed only by the bounded provider name and operation. Each record stores request count, available count, unavailable count, and total duration in milliseconds. The registry exposes `recordAiProviderCall` and renders stable Prometheus metric families.

`AiService` receives an optional shared `MetricsRegistry`. The composition root passes the runtime registry to the AI service; direct/unit construction remains source-compatible through a default null pointer. Each provider call is wrapped at the service boundary with a steady-clock timer, so disabled providers and injected fake providers are measured consistently. The existing result is returned unchanged even if metric recording is unavailable.

Metric labels are restricted to `provider` and `operation`, both normalized to a small allow-list (`disabled`, `http`, `unknown`; `diagnose`, `故障排查`, `日志摘要`, `unknown`) to prevent unbounded cardinality. No prompt or provider output is exported.

The HTTP metrics route remains unchanged. Existing counters and route metrics remain compatible, and the new families are additive.
