## ADDED Requirements

### Requirement: Versioned password verification
The system SHALL verify user credentials through a versioned password verification boundary instead of direct plaintext comparison.

#### Scenario: User logs in with PBKDF2 credential
- **WHEN** a stored credential uses the `pbkdf2_sha256$iterations$salt$hash` format and the user submits the matching password
- **THEN** the identity service authenticates the user and creates a session

#### Scenario: User submits wrong password
- **WHEN** a stored credential uses a supported password format and the user submits a non-matching password
- **THEN** the identity service rejects the login without creating a session

### Requirement: Development password compatibility
The system SHALL retain explicit development-only password compatibility for local demos and legacy in-memory fixtures.

#### Scenario: Demo user logs in with plain compatibility credential
- **WHEN** a stored credential uses the `plain:` compatibility format
- **THEN** the identity service can authenticate the demo user while keeping the compatibility boundary visible in stored data