## ADDED Requirements

### Requirement: Qt client work order synchronization
The system SHALL allow the Qt client to load maintenance work order rows from the backend after login.

#### Scenario: Work orders are loaded from backend
- **WHEN** the Qt client has a valid token and requests work orders
- **THEN** it displays rows returned by the backend work-order endpoint

### Requirement: Qt client work order transitions
The system SHALL allow the Qt client to submit basic work-order lifecycle transitions for a selected work order.

#### Scenario: User starts a selected work order
- **WHEN** an authenticated user selects a work order and starts processing
- **THEN** the client calls the backend start endpoint and refreshes the work-order list

#### Scenario: User completes a selected work order
- **WHEN** an authenticated user selects a work order and provides a completion result
- **THEN** the client calls the backend complete endpoint and refreshes the work-order list

### Requirement: Qt client work order fallback
The system SHALL keep local work-order demo rows available when backend calls fail.

#### Scenario: Backend work-order request fails
- **WHEN** the Qt client cannot synchronize or transition work orders
- **THEN** it keeps local demo rows visible and shows an offline fallback message