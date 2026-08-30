## ADDED Requirements

### Requirement: Redis readiness covers every selected authentication dependency

The system SHALL treat Redis as a required readiness dependency when either Redis-backed session storage or the Redis-backed distributed login-failure limiter is selected.

#### Scenario: Limiter-only Redis dependency is unavailable

- **GIVEN** `redis.session_store=memory` and `security.login_rate_limit_store=redis`
- **WHEN** Redis cannot be reached
- **THEN** `/health/live` SHALL remain available
- **AND** `/health/ready` SHALL return HTTP `503`
- **AND** Redis SHALL be marked required and unavailable

#### Scenario: Memory authentication controls do not require Redis

- **GIVEN** `redis.session_store=memory` and `security.login_rate_limit_store=memory`
- **WHEN** readiness is evaluated
- **THEN** Redis SHALL not be a required dependency
