## ADDED Requirements

### Requirement: MongoDB storage configuration is validated before runtime construction
The system SHALL reject MongoDB AI storage configuration with an empty database name, or with an empty URI and invalid host/port, before constructing the HTTP runtime.

#### Scenario: MongoDB URI fallback

- **GIVEN** `storage.ai_interaction_store=mongodb` and `mongodb.uri` is empty
- **AND** `mongodb.host` and `mongodb.port` are valid
- **WHEN** the HTTP composition root creates the MongoDB repository
- **THEN** it SHALL use the configured host and port as the connection endpoint

#### Scenario: Invalid MongoDB target

- **GIVEN** `storage.ai_interaction_store=mongodb`
- **WHEN** the database is empty, or the URI is empty with an empty host or port outside `1..65535`
- **THEN** configuration validation SHALL fail before the HTTP listener starts
