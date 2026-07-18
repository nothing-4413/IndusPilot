## ADDED Requirements

### Requirement: Alert HTTP API
The system SHALL expose authenticated HTTP endpoints for creating, listing, retrieving, and transitioning alert records.

#### Scenario: Authorized user creates an alert
- **WHEN** a user with `alert:write` submits a valid alert payload
- **THEN** the system stores the alert and returns the created record with severity and state

#### Scenario: Authorized user transitions alert lifecycle
- **WHEN** a user with `alert:write` acknowledges, assigns, resolves, or closes an alert
- **THEN** the system stores the new alert lifecycle state and related operator fields

#### Scenario: Authorized user retrieves alert detail
- **WHEN** a user with `alert:read` requests an alert by identifier
- **THEN** the system returns the alert record or a not-found response

### Requirement: Alert filtering
The system SHALL support filtering alert records by related asset, severity, and lifecycle state.

#### Scenario: User filters critical open alerts
- **WHEN** a user lists alerts with severity and state filters
- **THEN** the system returns only matching alert records