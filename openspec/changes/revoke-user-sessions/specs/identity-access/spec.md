## ADDED Requirements

### Requirement: Password rotation revokes user sessions
The system SHALL revoke all active sessions belonging to a user after successfully persisting a password rotation.

#### Scenario: All existing sessions are invalidated
- **WHEN** an authenticated user submits a valid current password and a compliant new password
- **THEN** every active session for that user, including the session used for the request, is rejected by protected endpoints

#### Scenario: A new session can be issued after revocation
- **WHEN** a user has successfully rotated a password and then authenticates with the new password
- **THEN** the system issues a new valid session

### Requirement: Session revocation failure is observable
The system SHALL not report a password rotation as fully successful when its session store cannot complete user-scoped revocation.

#### Scenario: Session store is unavailable during revocation
- **WHEN** the password hash update succeeds but the session store reports a revocation failure
- **THEN** the endpoint returns `SESSION_REVOCATION_FAILED` with HTTP 503 and records an audit failure without exposing credential or token data
