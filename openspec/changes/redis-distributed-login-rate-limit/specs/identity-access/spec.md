# Change: Distributed login-failure limiting

## Requirement: Distributed login-failure limiting

The system SHALL support a Redis-backed login-failure limiter that atomically shares per-username failure windows and lock state across backend instances.

### Scenario: Failed attempts are shared across instances

- **GIVEN** two identity-service instances use the same Redis limiter namespace
- **WHEN** failed attempts for one username reach the configured threshold across both instances
- **THEN** subsequent login attempts are rejected as locked by either instance
- **AND** the response includes the remaining lock duration through `Retry-After`

### Scenario: Successful login clears the failure state

- **GIVEN** a username has recorded failures but is not locked
- **WHEN** the username logs in successfully
- **THEN** the distributed failure state is cleared

### Scenario: Limiter storage failure fails closed

- **GIVEN** Redis is selected for login rate limiting
- **WHEN** the limiter cannot read or update its state
- **THEN** login is rejected with `AUTHENTICATION_RATE_LIMITER_UNAVAILABLE` and HTTP `503`

## Requirement: Backward-compatible local limiter

The system SHALL retain an in-memory login limiter as the default when distributed limiting is not configured.

### Scenario: Existing local lockout behavior remains available

- **GIVEN** `security.login_rate_limit_store` is `memory`
- **WHEN** failed attempts reach the configured threshold
- **THEN** the service returns the existing locked result without requiring Redis
