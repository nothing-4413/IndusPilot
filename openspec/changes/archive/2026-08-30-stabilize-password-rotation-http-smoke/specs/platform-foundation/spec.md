# Platform Foundation Smoke Delta

## ADDED Requirements

### Requirement: Password rotation smoke tolerates configured hashing cost

The HTTP integration smoke SHALL allocate a dedicated timeout budget for password rotation and the login requests that verify rotated credentials, sufficient for the configured PBKDF2 iteration count used by the test.

#### Scenario: Valid rotation completes under the configured hash cost
- **GIVEN** the smoke test configures PBKDF2 with 100,000 iterations
- **WHEN** it changes and restores a password and logs in with each resulting credential
- **THEN** the smoke test SHALL wait using the credential-operation timeout budget
- **AND** SHALL fail only when the operation exceeds that budget or returns an invalid response
