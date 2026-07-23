## ADDED Requirements

### Requirement: Qt client alert lifecycle actions
The system SHALL allow the Qt client to submit lifecycle actions for selected alerts through the backend.

#### Scenario: Operator acknowledges a selected alert
- **WHEN** an authenticated operator selects an alert and acknowledges it
- **THEN** the client calls the backend acknowledge endpoint and refreshes the alert list

#### Scenario: Operator resolves or closes a selected alert
- **WHEN** an authenticated operator selects an alert and resolves or closes it
- **THEN** the client calls the corresponding backend endpoint and refreshes the alert list

### Requirement: Qt client alert assignment
The system SHALL allow the Qt client to assign a selected alert through the backend.

#### Scenario: Operator assigns a selected alert
- **WHEN** an authenticated operator selects an alert and submits an assignee
- **THEN** the client calls the backend alert assignment endpoint and refreshes the alert list

### Requirement: Qt client alert action fallback
The system SHALL keep alert actions non-blocking when backend calls fail.

#### Scenario: Backend alert action request fails
- **WHEN** the Qt client cannot submit an alert action
- **THEN** it keeps the visible alert list available and shows a Chinese failure message