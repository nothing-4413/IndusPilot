# Change: Use authenticated MongoDB readiness checks

## ADDED Requirements

### Requirement: MongoDB readiness verifies authenticated command access

When MongoDB is required by the selected AI interaction store, readiness SHALL verify authenticated command-level access instead of relying only on TCP reachability.

#### Scenario: MongoDB credentials and database access are valid
- **GIVEN** `storage.ai_interaction_store=mongodb` and valid MongoDB credentials
- **WHEN** readiness probes dependencies
- **THEN** the MongoDB check SHALL execute an authenticated `ping` command
- **AND** SHALL report MongoDB available when the command succeeds

#### Scenario: MongoDB credentials or command access are invalid
- **GIVEN** MongoDB's TCP endpoint is reachable but authentication or database command access fails
- **WHEN** readiness probes dependencies
- **THEN** MongoDB SHALL be reported unavailable
- **AND** the readiness reason SHALL identify an authenticated MongoDB probe failure

#### Scenario: MongoDB probe is unavailable in the build
- **GIVEN** the backend was built without the MongoDB adapter
- **WHEN** configuration selects MongoDB AI storage
- **THEN** configuration validation SHALL reject startup before readiness probing
