## ADDED Requirements

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
