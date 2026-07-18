# industrial-assets Specification

## Purpose
TBD - created by archiving change define-induspilot-foundation. Update Purpose after archive.
## Requirements
### Requirement: Equipment asset records
The system SHALL manage equipment asset records with identity, type, location, status, ownership, and maintenance metadata.

#### Scenario: Equipment asset is created
- **WHEN** an authorized user creates an equipment asset with required fields
- **THEN** the system stores the asset and makes it available for monitoring, alerts, and work orders

### Requirement: Industrial hierarchy
The system SHALL represent factory, workshop, production line, and equipment hierarchy.

#### Scenario: Equipment is assigned to a production line
- **WHEN** a user links equipment to a production line
- **THEN** the system preserves the relationship for filtering, navigation, and reporting

### Requirement: Asset lifecycle status
The system SHALL track asset lifecycle status such as active, inactive, maintenance, and retired.

#### Scenario: Asset enters maintenance
- **WHEN** an authorized user marks an asset as under maintenance
- **THEN** the system reflects the maintenance status in asset and monitoring views

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

