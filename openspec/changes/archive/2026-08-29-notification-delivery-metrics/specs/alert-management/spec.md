# Change: Notification delivery metrics

## Requirement: Notification outcomes are observable

The system SHALL expose cumulative notification delivery outcome counters through `/metrics`.

### Scenario: Delivery outcome is classified

- **WHEN** a notification delivery attempt completes
- **THEN** metrics SHALL record its bounded channel and resulting queue outcome

### Scenario: Notification metrics do not leak business data

- **WHEN** notification metrics are rendered
- **THEN** labels SHALL contain only bounded channel and outcome values
- **AND** targets, messages, alert ids, rule ids, and credentials SHALL NOT appear

### Scenario: Existing delivery behavior remains compatible

- **WHEN** metrics recording is enabled or unavailable
- **THEN** queue retry, dead-letter, HTTP response, and audit behavior SHALL remain unchanged
