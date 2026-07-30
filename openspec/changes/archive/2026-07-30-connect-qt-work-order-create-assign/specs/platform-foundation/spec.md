## ADDED Requirements

### Requirement: Qt client work order creation
The system SHALL allow the Qt client to create maintenance work orders through the backend after login.

#### Scenario: Operator creates a work order
- **WHEN** an authenticated operator submits work-order id, asset id, and summary
- **THEN** the client calls the backend work-order creation endpoint and refreshes the work-order list

### Requirement: Qt client work order assignment
The system SHALL allow the Qt client to assign a selected maintenance work order through the backend.

#### Scenario: Operator assigns a selected work order
- **WHEN** an authenticated operator selects a work order and submits an assignee
- **THEN** the client calls the backend work-order assignment endpoint and refreshes the work-order list

### Requirement: Qt client work order creation fallback
The system SHALL keep work-order creation and assignment non-blocking when backend calls fail.

#### Scenario: Backend creation or assignment request fails
- **WHEN** the Qt client cannot create or assign a work order
- **THEN** it keeps the visible work-order list available and shows a Chinese failure message