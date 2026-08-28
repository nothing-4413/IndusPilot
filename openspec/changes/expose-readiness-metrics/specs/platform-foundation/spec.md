## ADDED Requirements

### Requirement: Readiness state is observable through Prometheus
The system SHALL expose the latest readiness state and probe lifecycle counters through the existing `/metrics` endpoint.

#### Scenario: Readiness metrics are rendered
- **WHEN** readiness has been evaluated and `/metrics` is requested
- **THEN** the response contains readiness state, probe, failure, recovery, duration, and timestamp metrics

#### Scenario: Readiness snapshots do not double count
- **WHEN** `/health/ready` is scraped repeatedly without a new probe
- **THEN** the exported cumulative counters remain equal to the application readiness snapshot
