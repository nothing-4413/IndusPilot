# Change: Webhook egress allowlist

## Requirement: Webhook delivery is host constrained

The system SHALL send webhook requests only to hosts explicitly listed in configuration.

### Scenario: Allowed webhook host receives delivery

- **GIVEN** webhook delivery is enabled and the target host exactly matches the allowlist
- **WHEN** the webhook sender delivers a notification
- **THEN** it MAY make the configured HTTP(S) request

### Scenario: Disallowed webhook host is rejected

- **GIVEN** webhook delivery is enabled and the target host is absent from the allowlist
- **WHEN** a notification is dispatched
- **THEN** the sender SHALL make no outbound request
- **AND** the notification SHALL follow normal retry/dead-letter behavior

### Scenario: Enabled webhook requires an allowlist

- **WHEN** startup configuration enables webhook delivery with an empty allowlist
- **THEN** static configuration validation SHALL fail before the HTTP listener starts
