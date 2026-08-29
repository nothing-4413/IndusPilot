# Change: Append-only operation audit storage

## Requirement: Operation audit events are append-only

The system SHALL never update or delete an existing operation audit event when an event code is reused.

### Scenario: Duplicate in-memory event is ignored

- **GIVEN** an operation audit event has already been stored
- **WHEN** another event with the same event code is recorded
- **THEN** the original event fields SHALL remain unchanged
- **AND** integrity verification SHALL continue to pass

### Scenario: Duplicate MySQL event is ignored

- **GIVEN** an operation audit event has already been stored in MySQL
- **WHEN** the repository receives the same event code again
- **THEN** the database write SHALL not update the existing row
- **AND** the persisted original event SHALL be returned

### Scenario: Distinct events remain appendable

- **WHEN** a new event code is recorded
- **THEN** it SHALL be appended with its calculated previous hash and event hash
