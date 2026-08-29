# Change: Pin webhook connections to validated addresses

## Requirement: Webhook transport must use a validated address

The system SHALL connect webhook requests to an address returned by the same public-address validation performed immediately before delivery.

### Scenario: DNS changes after validation

- **GIVEN** a webhook hostname resolves to a permitted public address during validation
- **WHEN** the notification is sent
- **THEN** the HTTP client SHALL use the validated address rather than resolving the hostname again

### Scenario: HTTP virtual host is preserved

- **GIVEN** a webhook target uses an HTTP hostname
- **WHEN** delivery is sent to its validated address
- **THEN** the request Host header SHALL contain the original hostname and effective port

### Scenario: TLS validation is not weakened

- **GIVEN** a webhook target uses HTTPS
- **WHEN** the pinned client is created
- **THEN** certificate validation SHALL remain enabled
- **AND** delivery SHALL fail rather than disable validation when the connector cannot provide required hostname SNI
