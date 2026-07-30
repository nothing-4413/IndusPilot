## MODIFIED Requirements

### Requirement: Runtime repository selection
The system SHALL support explicit runtime repository selection for services that have multiple repository implementations.

#### Scenario: Runtime selects MySQL repositories for operational modules
- **WHEN** the repository store is configured as `mysql`
- **THEN** identity, asset, alert, work-order, runtime-state, and AI-interaction services use configured MySQL repositories where implementations exist