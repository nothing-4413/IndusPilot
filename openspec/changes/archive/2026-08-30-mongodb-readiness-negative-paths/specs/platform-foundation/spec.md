## ADDED Requirements

### Requirement: MongoDB readiness failures remain observable and sanitized
The MongoDB-backed HTTP runtime SHALL remain alive when an authenticated MongoDB probe temporarily fails, SHALL return HTTP `503` from `/health/ready`, and SHALL NOT expose the configured URI, username, or password in readiness diagnostics.

#### Scenario: MongoDB authentication failure

- **GIVEN** `storage.ai_interaction_store=mongodb` and the configured MongoDB password is invalid
- **WHEN** an operator requests `/health/ready`
- **THEN** the HTTP runtime SHALL remain live
- **AND** readiness SHALL return `503`
- **AND** MongoDB SHALL be marked required and unavailable
- **AND** the response SHALL NOT contain the MongoDB URI, username, or password

#### Scenario: MongoDB dependency recovery

- **GIVEN** MongoDB readiness previously failed
- **WHEN** the configured dependency is restored and the readiness cache expires
- **THEN** readiness SHALL return `200`
- **AND** MongoDB SHALL be marked available

#### Scenario: Non-positive ping response

- **GIVEN** an authenticated MongoDB `ping` response contains a numeric `ok` value less than or equal to zero
- **WHEN** the response is evaluated
- **THEN** the probe SHALL report unavailable with a stable failure reason

#### Scenario: MongoDB business database permission failure

- **GIVEN** the MongoDB account can authenticate and run `ping` but cannot create or update the AI interaction indexes
- **WHEN** the MongoDB AI repository coordinates its indexes
- **THEN** startup SHALL fail with a non-zero result
- **AND** diagnostics SHALL NOT contain the MongoDB URI, username, or password
