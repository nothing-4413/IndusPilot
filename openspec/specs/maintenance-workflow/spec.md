# maintenance-workflow Specification

## Purpose
TBD - created by archiving change define-induspilot-foundation. Update Purpose after archive.
## Requirements
### Requirement: Work order lifecycle
The system SHALL manage maintenance work orders through creation, assignment, processing, completion, and closure.

#### Scenario: Work order is assigned
- **WHEN** a dispatcher assigns a work order to a maintainer
- **THEN** the system records the assignee and updates the work order state

### Requirement: Work order relationship to alerts and assets
The system SHALL allow work orders to reference related alerts and equipment assets.

#### Scenario: Work order is created from alert
- **WHEN** an operator creates a work order from an alert
- **THEN** the system links the work order to the source alert and equipment asset

### Requirement: Maintenance history
The system SHALL preserve completed maintenance records for future asset review and AI-assisted diagnosis.

#### Scenario: Asset maintenance history is reviewed
- **WHEN** a user opens an asset maintenance history
- **THEN** the system lists related closed work orders and maintenance outcomes

