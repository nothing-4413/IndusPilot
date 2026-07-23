## ADDED Requirements

### Requirement: Work order HTTP API
The system SHALL expose authenticated HTTP endpoints for creating, listing, retrieving, and transitioning maintenance work orders.

#### Scenario: Authorized user creates a work order
- **WHEN** a user with `work-order:write` submits a valid work order payload
- **THEN** the system stores the work order and returns the created record

#### Scenario: Authorized user transitions work order lifecycle
- **WHEN** a user with `work-order:write` assigns, starts, completes, or closes a work order
- **THEN** the system stores the new work order state and related execution fields

### Requirement: Work order creation from alert API
The system SHALL allow authorized users to create work orders from existing alerts through HTTP.

#### Scenario: User creates work order from alert
- **WHEN** a user with `work-order:write` submits an existing alert identifier and summary
- **THEN** the system creates a work order linked to the source alert and asset

### Requirement: Asset maintenance history API
The system SHALL expose completed and closed maintenance history for an equipment asset.

#### Scenario: User views asset maintenance history
- **WHEN** a user with `work-order:read` requests maintenance history for an asset
- **THEN** the system returns closed work orders linked to that asset