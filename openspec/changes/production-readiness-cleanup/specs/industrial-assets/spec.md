## ADDED Requirements

### Requirement: Configured asset repository runtime
The system SHALL run the asset service with the configured asset repository implementation.

#### Scenario: Asset service starts with MySQL repository mode
- **WHEN** repository store is configured as `mysql`
- **THEN** the asset service uses the MySQL asset repository for asset reads and writes