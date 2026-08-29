# Change: AI provider runtime metrics

## Requirement: AI provider calls are observable

The system SHALL expose cumulative AI provider call counters and latency totals through the existing Prometheus metrics endpoint.

### Scenario: Provider result is classified

- **WHEN** an AI service operation completes through any provider
- **THEN** metrics SHALL record the bounded provider name, operation, availability result, request count, and elapsed duration

### Scenario: Provider metrics do not leak request data

- **WHEN** `/metrics` is rendered
- **THEN** provider metric labels SHALL contain only bounded provider and operation values
- **AND** prompts, responses, credentials, endpoints, and user identifiers SHALL NOT appear in those metrics

### Scenario: Existing AI behavior remains compatible

- **WHEN** provider metrics are recorded
- **THEN** the existing AI result, local fallback, interaction audit, and `/metrics` response contracts SHALL remain available
