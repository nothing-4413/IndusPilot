## ADDED Requirements

### Requirement: Selected Redis authentication features require compiled support

The system SHALL reject Redis-backed session storage and Redis-backed login rate limiting before startup when the binary was not built with `INDUSPILOT_WITH_REDIS`.

#### Scenario: Redis session store selected in a no-Redis binary

- **GIVEN** a binary without `INDUSPILOT_WITH_REDIS`
- **WHEN** `redis.session_store=redis` is configured
- **THEN** static configuration validation SHALL fail
- **AND** the HTTP listener SHALL NOT start
- **AND** the runtime SHALL NOT substitute an in-memory session store

#### Scenario: Redis feature selection in a Redis-enabled binary

- **GIVEN** a binary built with `INDUSPILOT_WITH_REDIS`
- **WHEN** Redis-backed session storage or login rate limiting is configured with a URI
- **THEN** static configuration validation SHALL allow the selection

### Requirement: HTTP static configuration is validated before service construction

The HTTP runtime SHALL validate static configuration before constructing its service context.

#### Scenario: Unsupported feature selection

- **WHEN** a static configuration validation fails
- **THEN** the runtime SHALL report configuration errors and exit with the existing configuration failure status
- **AND** it SHALL NOT construct external clients or service implementations
