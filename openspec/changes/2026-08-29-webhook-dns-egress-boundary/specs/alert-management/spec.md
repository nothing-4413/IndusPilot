# Change: Resolved webhook egress boundary

## Requirement: Webhook delivery requires a public resolved address

The system SHALL resolve an allowlisted webhook host before transport and SHALL reject the delivery if any resolved stream address is non-public, unsupported, or unavailable.

### Scenario: Private resolved address is rejected

- **GIVEN** webhook delivery is enabled and the hostname is allowlisted
- **AND** the hostname resolves to a loopback or private address
- **WHEN** a notification is dispatched
- **THEN** no HTTP client request SHALL be made
- **AND** the notification SHALL follow normal retry/dead-letter behavior

### Scenario: Public hostname remains usable

- **GIVEN** webhook delivery is enabled and every resolved address is globally routable
- **WHEN** a notification is dispatched
- **THEN** the existing timeout-bounded HTTP request MAY be made using the original hostname

### Scenario: Malformed or unsupported endpoint is rejected

- **WHEN** a webhook target has invalid authority, port, or address resolution
- **THEN** delivery SHALL fail before transport
- **AND** the error SHALL not expose credentials or the allowlist
