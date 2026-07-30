## ADDED Requirements

### Requirement: Qt client monitoring state synchronization
The system SHALL allow the Qt client to load runtime monitoring state rows from the backend after login.

#### Scenario: Monitoring states are loaded from backend
- **WHEN** the Qt client has a valid token and requests monitoring states
- **THEN** it displays rows returned by the backend monitoring endpoint

### Requirement: Qt client alert list synchronization
The system SHALL allow the Qt client to load alert rows from the backend after login.

#### Scenario: Alerts are loaded from backend
- **WHEN** the Qt client has a valid token and requests alerts
- **THEN** it displays rows returned by the backend alert endpoint

### Requirement: Qt client monitoring and alert fallback
The system SHALL keep local monitoring and alert demo rows available when backend calls fail.

#### Scenario: Backend list request fails
- **WHEN** the Qt client cannot synchronize monitoring states or alerts
- **THEN** it keeps local demo rows visible and shows an offline fallback mode for that page