## ADDED Requirements

### Requirement: Qt client HTTP login
The system SHALL allow the Qt client to authenticate through the backend HTTP login endpoint when the backend is reachable.

#### Scenario: User logs in from Qt client with backend available
- **WHEN** a user submits credentials in the Qt client and the configured backend accepts them
- **THEN** the client stores the returned session token for subsequent API requests
- **AND** the UI indicates that it is connected to the backend

### Requirement: Qt client asset list synchronization
The system SHALL allow the Qt client to load equipment assets from the backend HTTP asset endpoint after login.

#### Scenario: Asset list is loaded from backend
- **WHEN** the Qt client has a valid token and requests the asset list
- **THEN** it displays asset rows returned by the backend

### Requirement: Qt client offline fallback
The system SHALL keep offline demo data available when backend HTTP calls fail.

#### Scenario: Backend is unavailable
- **WHEN** the Qt client cannot reach the configured backend or receives an invalid response
- **THEN** it keeps local demo data visible and shows an offline fallback message