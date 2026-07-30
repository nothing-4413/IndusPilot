## ADDED Requirements

### Requirement: Database schema version baseline
The system SHALL provide a repeatable schema version baseline for MySQL initialization scripts.

#### Scenario: MySQL initialization scripts are applied
- **WHEN** the MySQL initialization scripts are executed in order
- **THEN** the database records the applied schema versions in a dedicated migration tracking table

### Requirement: Deployment preflight checks
The system SHALL provide a local deployment preflight check for runtime dependency readiness.

#### Scenario: Operator runs preflight before starting production-like backend
- **WHEN** an operator runs the preflight script from the repository root
- **THEN** the script checks required configuration, database scripts, schema version declarations, and compose dependency definitions
- **AND** it returns a non-zero exit code when required checks fail