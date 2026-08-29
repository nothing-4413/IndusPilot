# Change: Govern MySQL migration execution

## Requirement: MySQL migrations must have a controlled execution path

The system SHALL provide a documented migration runner that applies local MySQL migration scripts in numeric order and verifies their schema version registrations.

### Scenario: Apply pending migrations

- **GIVEN** the MySQL server is reachable and migration scripts are available
- **WHEN** the migration runner is invoked
- **THEN** pending scripts SHALL be applied in numeric order
- **AND** each successfully applied script SHALL be present in `schema_migrations`

### Scenario: Reject unknown recorded versions

- **GIVEN** `schema_migrations` contains a version with no matching local migration file
- **WHEN** the migration runner is invoked
- **THEN** it SHALL fail before applying a later migration

### Scenario: Reject an out-of-order schema

- **GIVEN** a later local migration is recorded while an earlier local migration is missing
- **WHEN** the migration runner is invoked
- **THEN** it SHALL fail and identify the missing earlier version

### Scenario: Re-run an up-to-date database

- **GIVEN** every local migration version is already recorded
- **WHEN** the migration runner is invoked again
- **THEN** it SHALL succeed without reapplying pending migrations
