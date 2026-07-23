# alert-management Specification

## Purpose
定义告警记录、严重度、确认、分派、解决和关闭生命周期，用于支撑工业现场异常事件从发现到闭环处置的完整流程。
## Requirements
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

