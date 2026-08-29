# Change: Truthful notification channel delivery

## Requirement: Notification delivery reports real channel outcomes

The system SHALL mark a notification as sent only when its configured channel sender confirms delivery.

### Scenario: Unsupported channel is not reported as sent

- **WHEN** a notification uses an unavailable channel adapter
- **THEN** delivery SHALL enter the existing retrying or dead-letter state
- **AND** the notification SHALL retain a failure reason

### Scenario: Webhook delivery is explicitly enabled

- **GIVEN** webhook delivery is enabled and the target is a valid HTTP(S) URL
- **WHEN** the sender receives a 2xx response within the configured timeout
- **THEN** the notification SHALL be marked `sent`

### Scenario: Webhook failure is retryable

- **WHEN** an enabled webhook times out, cannot connect, or returns a non-2xx response
- **THEN** the notification SHALL follow bounded retry and dead-letter behavior

### Scenario: Webhook delivery is disabled by default

- **GIVEN** webhook delivery is not explicitly enabled
- **WHEN** a webhook notification is dispatched
- **THEN** it SHALL not make an outbound request
- **AND** it SHALL be reported as unavailable through normal queue failure metadata
