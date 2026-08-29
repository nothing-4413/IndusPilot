# Change: Align configurable MySQL database names

## Requirement: MySQL database selection must be consistent

The system SHALL use the configured MySQL database name for migration execution, dependency smoke, and backend repository access.

### Scenario: Initialize a custom database

- **GIVEN** `MYSQL_DATABASE` is set to a valid identifier other than `induspilot`
- **WHEN** the migration runner is invoked
- **THEN** all migrations SHALL execute in that database
- **AND** `schema_migrations` SHALL be recorded in that database

### Scenario: Verify a custom database

- **GIVEN** Compose starts MySQL with a configured database name
- **WHEN** dependency smoke runs
- **THEN** migration checks and real CRUD smoke SHALL query that same database

### Scenario: Reject an unsafe database identifier

- **GIVEN** `MYSQL_DATABASE` contains characters outside the supported identifier pattern
- **WHEN** the migration runner is invoked
- **THEN** it SHALL fail before creating or applying schema changes

### Scenario: Default database remains compatible

- **GIVEN** no database override is configured
- **WHEN** deployment is initialized
- **THEN** the database name SHALL remain `induspilot`
