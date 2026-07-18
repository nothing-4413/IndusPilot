# operational-monitoring Specification

## Purpose
TBD - created by archiving change define-induspilot-foundation. Update Purpose after archive.
## Requirements
### Requirement: Equipment status overview
The system SHALL provide an overview of equipment operational status for dashboard and list views.

#### Scenario: Operator opens monitoring dashboard
- **WHEN** an operator opens the monitoring dashboard
- **THEN** the system displays summarized equipment status grouped by operational state and severity

### Requirement: Runtime state separation
The system SHALL distinguish durable asset metadata from short-lived runtime state.

#### Scenario: Runtime state changes
- **WHEN** equipment runtime state changes
- **THEN** the system updates the monitoring state without altering durable asset identity metadata

### Requirement: Real-time update channel
The system SHALL define a real-time update channel for monitoring-facing state changes.

#### Scenario: Equipment state update is pushed
- **WHEN** a monitored equipment state changes
- **THEN** connected clients receive an update without requiring a full manual refresh

