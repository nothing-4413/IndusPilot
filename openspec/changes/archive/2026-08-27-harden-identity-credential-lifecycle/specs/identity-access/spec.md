## ADDED Requirements

### Requirement: Configurable password policy
The system SHALL load and validate minimum password length and PBKDF2 iteration count from configuration or environment variables before starting the listener.

#### Scenario: Unsafe password policy is rejected
- **WHEN** the configured minimum length or PBKDF2 iteration count is below the production floor
- **THEN** configuration validation fails and the listener does not start

### Requirement: Authenticated password rotation
The system SHALL allow an authenticated user to replace their password after verifying the current password and the configured password policy.

#### Scenario: Password rotation succeeds
- **WHEN** a valid session submits the correct current password and a compliant new password
- **THEN** the new password is stored as a versioned PBKDF2-SHA256 hash and the endpoint returns success

#### Scenario: Password rotation is rejected
- **WHEN** the current password is incorrect or the new password violates policy
- **THEN** the endpoint returns a safe failure response and does not change the stored hash

### Requirement: Credential data is excluded from operational records
The system SHALL not include plaintext passwords or password hashes in HTTP logs, response bodies, or password rotation audit events.

#### Scenario: Password rotation is audited
- **WHEN** a password rotation succeeds or fails
- **THEN** an audit event records the actor, user resource, result and trace id without credential values
