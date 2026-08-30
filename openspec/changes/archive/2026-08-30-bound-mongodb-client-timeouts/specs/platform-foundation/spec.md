## ADDED Requirements

### Requirement: MongoDB AI client connection attempts are bounded
The MongoDB AI repository SHALL use the configured readiness probe timeout as the upper bound for MongoDB server selection and connection attempts.

#### Scenario: Repository starts with a slow or unavailable MongoDB

- **GIVEN** `storage.ai_interaction_store=mongodb` and `readiness.probe_timeout_ms` is configured
- **WHEN** the repository creates its MongoDB client or retries deferred index reconciliation
- **THEN** server selection and connection options SHALL use the configured timeout
- **AND** the runtime SHALL not fall back to the driver's unbounded default timeout
