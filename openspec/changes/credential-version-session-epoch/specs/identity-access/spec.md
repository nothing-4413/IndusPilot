# Change: Credential-version session epochs

## Requirement: Credential-versioned sessions

The system SHALL associate each authenticated session with the user's credential version at issuance time and SHALL reject the session when the current credential version differs.

### Scenario: Password rotation invalidates every prior session

- **GIVEN** a user has one or more valid sessions
- **WHEN** the user's password is rotated successfully
- **THEN** the user's credential version is incremented atomically with the new password hash
- **AND** every session carrying the previous version is rejected by session validation

### Scenario: New password receives a new session epoch

- **GIVEN** a user has completed password rotation
- **WHEN** the user logs in with the new password
- **THEN** the new session carries the current credential version and validates successfully

### Scenario: Legacy session value is fail-closed

- **GIVEN** a Redis session value uses the pre-versioned `v1` format
- **WHEN** the identity service validates that session
- **THEN** the value can be decoded for cleanup but does not pass credential-version validation

## Requirement: Atomic credential version persistence

The system SHALL update a user's password hash and credential version as one repository operation so a successful password rotation cannot issue sessions with an ambiguous version.

### Scenario: Disabled or unknown user is not updated

- **WHEN** a password update targets an unknown or disabled user
- **THEN** the repository reports failure and neither credential value nor version is changed

## Requirement: Safe Redis session revocation scan

The system SHALL not issue string reads against Redis user-session index keys while scanning session keys for compatibility cleanup.

### Scenario: Indexed and legacy sessions are revoked

- **WHEN** user-scoped revocation runs against Redis
- **THEN** indexed session keys and legacy unindexed session keys for that user are deleted
- **AND** sorted-set index keys are handled only as sorted sets
