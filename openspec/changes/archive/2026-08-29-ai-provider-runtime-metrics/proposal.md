# Change: Expose AI provider runtime metrics

## Why

The AI layer already records HTTP request totals and AI interaction audits, but operators cannot distinguish provider availability, fallback outcomes, operations, or latency from those aggregate signals. This makes provider incidents difficult to diagnose without reading application logs.

## What Changes

- Add an injectable AI provider metrics sink to the AI service.
- Record every provider completion with provider, operation, availability, and elapsed time.
- Export bounded Prometheus counters and latency sums through the existing `/metrics` endpoint.
- Preserve the current AI response, fallback, audit, and HTTP contracts.
- Add unit and HTTP smoke coverage plus operational documentation.

## Non-Goals

- No provider-specific business payload changes.
- No raw prompt, response, API key, endpoint, or user data in metric labels.
- No change to readiness semantics or provider retry policy.
