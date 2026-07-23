## ADDED Requirements

### Requirement: Runtime repository selection
The system SHALL support explicit runtime repository selection for services that have multiple repository implementations.

#### Scenario: Default runtime uses memory repositories
- **WHEN** no repository store is configured
- **THEN** the system uses memory repositories so local development and integration tests remain self-contained

#### Scenario: Runtime selects MySQL repositories
- **WHEN** the repository store is configured as `mysql`
- **THEN** services with MySQL repository implementations use configured MySQL connectivity instead of memory repositories