# Design: Readiness Prometheus metrics

The existing `/health/ready` route calls `Application::readiness()` and will pass the returned snapshot to `MetricsRegistry::recordReadiness`. The registry stores only aggregate values, so dependency names and configuration details do not become metric labels.

The registry records the current `ready` value as `0` or `1`, the probe count, failure count, recovery count, latest probe duration in milliseconds, and latest probe Unix timestamp. Since readiness snapshots are cumulative, the registry replaces these values with the latest snapshot rather than incrementing them on every scrape.

The output uses stable Prometheus names: `induspilot_readiness`, `induspilot_readiness_probes_total`, `induspilot_readiness_failures_total`, `induspilot_readiness_recoveries_total`, `induspilot_readiness_probe_duration_ms`, and `induspilot_readiness_last_probe_at_unix_ms`.
