# Change: Protect MySQL migration integrity and concurrency

## Requirement: MySQL migration history must bind scripts to checksums

The system SHALL record and verify the SHA-256 checksum of each applied local MySQL migration.

### Scenario: Existing script is modified

- **GIVEN** a migration version has a recorded non-empty checksum
- **WHEN** its local SQL content differs from that checksum
- **THEN** the migration runner SHALL fail before applying pending migrations

### Scenario: Upgrade from version-only history

- **GIVEN** migration history predates checksum support
- **WHEN** the checksum schema migration succeeds
- **THEN** the runner SHALL backfill checksums for known recorded local migrations

## Requirement: MySQL migration execution must be serialized

The system SHALL hold a MySQL advisory lock in the same database session used for migration execution.

### Scenario: Concurrent migration runner

- **GIVEN** another runner holds the migration advisory lock
- **WHEN** a second runner starts
- **THEN** it SHALL fail before applying a migration

### Scenario: Migration completion

- **WHEN** a migration runner completes or fails
- **THEN** it SHALL release its advisory lock and close the session
