# Change: Operation audit time-range filters

## Requirement: Operation audit queries support time windows

The system SHALL support inclusive occurrence-time filters for operation audit events and audit CSV exports.

### Scenario: Audit events are filtered by an inclusive range

- **GIVEN** an authorized administrator requests `occurredFrom` and `occurredTo`
- **WHEN** audit events are queried
- **THEN** only events whose `occurredAt` is within the inclusive range SHALL be returned
- **AND** actor, action, resource type, result, and pagination filters SHALL continue to apply

### Scenario: Invalid audit time range is rejected

- **WHEN** a query contains a malformed timestamp or an earlier `occurredFrom` than `occurredTo`
- **THEN** the endpoint SHALL return `400 INVALID_REQUEST`
- **AND** it SHALL not export or return audit rows

### Scenario: Audit export reuses time filters

- **GIVEN** an authorized administrator requests a CSV export with a valid time range
- **THEN** the CSV SHALL contain only matching events
- **AND** the export response format and audit integrity fields SHALL remain unchanged
