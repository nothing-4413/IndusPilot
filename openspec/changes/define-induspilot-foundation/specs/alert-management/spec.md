## ADDED Requirements

### Requirement: Alert record lifecycle
The system SHALL manage alert records from creation through acknowledgement, assignment, resolution, and closure.

#### Scenario: Alert is acknowledged
- **WHEN** an operator acknowledges an open alert
- **THEN** the system records the acknowledgement user and timestamp

### Requirement: Alert severity
The system SHALL classify alerts by severity and expose severity in list, detail, dashboard, and notification contexts.

#### Scenario: Critical alert appears
- **WHEN** a critical alert is created
- **THEN** the system marks it with critical severity and prioritizes it in alert views

### Requirement: Alert relationship to assets
The system SHALL associate alerts with equipment assets when the source equipment is known.

#### Scenario: Asset alert is viewed
- **WHEN** a user opens an alert linked to equipment
- **THEN** the system shows the related asset context and navigation path
