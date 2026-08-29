# Change: Durable notification delivery queue

## Requirement: Concurrent notification dispatch is leased

The system SHALL atomically claim due notification records with a short-lived lease before delivery.

### Scenario: Concurrent workers do not claim the same record

- **GIVEN** two workers dispatch the same queued notification concurrently
- **WHEN** both request due notification claims
- **THEN** at most one worker receives the record in its claimed batch

### Scenario: Expired leases become eligible again

- **GIVEN** a notification lease expires before delivery completes
- **WHEN** a later dispatch requests due records
- **THEN** the notification can be claimed again

## Requirement: Delivery retries are bounded and scheduled

The system SHALL schedule transient notification failures with bounded exponential backoff and SHALL stop automatic retries after the configured maximum attempts.

### Scenario: Failed delivery is delayed

- **WHEN** delivery fails before the maximum attempt count
- **THEN** the notification is `retrying`
- **AND** its next attempt time is in the future

### Scenario: Exhausted delivery becomes dead letter

- **WHEN** delivery fails at the maximum attempt count
- **THEN** the notification is `dead_letter`
- **AND** ordinary dispatch does not select it again
