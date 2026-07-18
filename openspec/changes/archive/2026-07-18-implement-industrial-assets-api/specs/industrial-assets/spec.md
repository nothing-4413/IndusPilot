## ADDED Requirements

### Requirement: Asset HTTP API
The system SHALL expose authenticated HTTP endpoints for creating, listing, retrieving, and updating equipment asset lifecycle state.

#### Scenario: Authorized user creates an asset through HTTP
- **WHEN** a user with `asset:write` submits a valid equipment asset payload
- **THEN** the system stores the asset through the asset repository and returns the created record

#### Scenario: Authorized user retrieves an asset through HTTP
- **WHEN** a user with `asset:read` requests an existing equipment asset by identifier
- **THEN** the system returns the durable asset metadata

#### Scenario: Authorized user changes asset lifecycle state
- **WHEN** a user with `asset:write` updates an asset lifecycle state
- **THEN** the system stores the new state without changing the asset identity or hierarchy metadata

### Requirement: Asset hierarchy filtering
The system SHALL support filtering equipment assets by factory, workshop, production line, and lifecycle status.

#### Scenario: User filters assets by production hierarchy
- **WHEN** a user lists assets with hierarchy filter parameters
- **THEN** the system returns only assets matching the requested hierarchy values

#### Scenario: User filters assets by lifecycle state
- **WHEN** a user lists assets with a lifecycle status filter
- **THEN** the system returns only assets currently in that lifecycle state
