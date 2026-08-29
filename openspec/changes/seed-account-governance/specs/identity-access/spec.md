# Change: Seed account governance

## Requirement: Seed credentials require explicit governance

The system SHALL identify published seed accounts as requiring password rotation and SHALL reject ordinary sign-in for those accounts unless an explicit development compatibility setting is enabled.

### Scenario: Production-oriented login rejects an unrotated seed account

- **GIVEN** a user credential requires password rotation
- **AND** `security.allow_seed_credentials` is disabled
- **WHEN** the user submits otherwise valid sign-in credentials
- **THEN** no session is issued
- **AND** the response code is `SEED_CREDENTIAL_ROTATION_REQUIRED` with HTTP `403`

### Scenario: Local demo explicitly permits demonstration credentials

- **GIVEN** a user credential requires password rotation
- **AND** `security.allow_seed_credentials` is enabled
- **WHEN** the user submits valid credentials
- **THEN** normal authentication behavior remains available

## Requirement: Controlled seed credential rotation

The deployment tooling SHALL allow an operator with database access to rotate a named seed credential without recording the supplied password or resulting hash.

### Scenario: Rotation clears the governance state

- **GIVEN** a seed user requires password rotation
- **WHEN** the deployment rotation utility persists a compliant replacement password
- **THEN** the stored credential uses a newly generated PBKDF2-SHA256 salt and hash
- **AND** the credential version is incremented
- **AND** the user no longer requires password rotation

### Scenario: Existing non-seed credentials are not overwritten by migration

- **GIVEN** an existing user has a password hash different from the published seed hash
- **WHEN** the seed governance migration runs
- **THEN** its password hash and password-rotation state are unchanged
