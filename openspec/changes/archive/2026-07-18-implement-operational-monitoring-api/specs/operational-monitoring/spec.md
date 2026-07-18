## ADDED Requirements

### Requirement: Runtime state HTTP API
The system SHALL expose authenticated HTTP endpoints for listing, retrieving, and updating short-lived equipment runtime state.

#### Scenario: Authorized user lists runtime states
- **WHEN** a user with `asset:read` opens the monitoring state list
- **THEN** the system returns current runtime states and a summary grouped by state

#### Scenario: Authorized user retrieves runtime state for one asset
- **WHEN** a user with `asset:read` requests runtime state by asset identifier
- **THEN** the system returns the current runtime state or a not-found response

#### Scenario: Authorized user updates runtime state
- **WHEN** a user with `monitoring:write` submits a runtime state payload
- **THEN** the system updates the runtime state without modifying durable asset metadata

### Requirement: Monitoring write permission
The system SHALL distinguish monitoring runtime-state write permission from asset master-data write permission.

#### Scenario: User without monitoring write permission updates runtime state
- **WHEN** a user has `asset:read` but lacks `monitoring:write`
- **THEN** the system rejects the runtime-state update with an authorization error