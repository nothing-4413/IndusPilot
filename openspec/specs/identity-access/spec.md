# identity-access Specification

## Purpose
定义用户认证、会话管理、角色权限映射和受保护接口访问边界，确保后端业务模块能够统一执行身份与授权检查。
## Requirements
### Requirement: User authentication
The system SHALL allow users to authenticate before accessing protected industrial operations features.

#### Scenario: User logs in successfully
- **WHEN** a valid user submits correct credentials
- **THEN** the system creates an authenticated session and allows access to authorized features

### Requirement: Role-based access control
The system SHALL restrict operations by user role and permission.

#### Scenario: Unauthorized operation is blocked
- **WHEN** a user without the required permission attempts a protected operation
- **THEN** the system denies the operation and records the denial reason

### Requirement: Session management
The system SHALL support session creation, validation, expiration, and logout. Expired or inactive in-memory session entries SHALL be removed when read, and non-positive session TTL saves SHALL remove the entry. Successful logout SHALL produce an `auth.logout` audit event without the session token; storage failures after validation SHALL be reported as unavailable rather than unauthorized.

#### Scenario: Expired session is rejected
- **WHEN** a user sends a request with an expired session
- **THEN** the system rejects the request and requires re-authentication

#### Scenario: Successful logout is audited
- **GIVEN** a user sends a request with a valid session
- **WHEN** the user logs out
- **THEN** the session is revoked
- **AND** an `auth.logout` audit event records the user and trace id without the session token

#### Scenario: Logout revocation storage failure is unavailable
- **GIVEN** a user sends a request with a valid session
- **AND** the session store cannot revoke the session
- **WHEN** the user logs out
- **THEN** the endpoint returns HTTP 503

#### Scenario: Expired in-memory session is reclaimed
- **GIVEN** an in-memory session expires or is saved with a non-positive TTL
- **WHEN** the session is read
- **THEN** no session is returned
- **AND** the expired entry is not retained by the session store

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

### Requirement: 会话 Token 不可预测

系统 SHALL 使用不可预测的随机值生成会话 Token，且 Token 不得包含用户名、时间戳或递增计数器等可推断信息。

#### Scenario: 用户登录后获得随机会话 Token

- **GIVEN** 用户提供正确账号和密码
- **WHEN** 身份服务创建会话
- **THEN** 返回的 Token SHALL contain at least 256 bits of random material
- **AND** Token SHALL NOT contain the username
- **AND** 连续两次成功登录 SHALL produce different tokens

### Requirement: Repository-backed identity lookup
The system SHALL authenticate users and resolve role permissions through configured identity repositories.

#### Scenario: User logs in through repository-backed identity
- **WHEN** the identity service receives a login request
- **THEN** it retrieves the user credential from the configured user repository and resolves permissions from the configured permission repository

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

### Requirement: Credential data is excluded from operational records
The system SHALL not include plaintext passwords or password hashes in HTTP logs, response bodies, or password rotation audit events.

#### Scenario: Password rotation is audited
- **WHEN** a password rotation succeeds or fails
- **THEN** an audit event records the actor, user resource, result and trace id without credential values

### Requirement: Credential-versioned sessions

The system SHALL associate each authenticated session with the user's credential version at issuance time and SHALL reject the session when the current credential version differs.

#### Scenario: Password rotation invalidates every prior session
- **GIVEN** a user has one or more valid sessions
- **WHEN** the user's password is rotated successfully
- **THEN** the user's credential version is incremented atomically with the new password hash
- **AND** every session carrying the previous version is rejected by session validation

#### Scenario: New password receives a new session epoch
- **GIVEN** a user has completed password rotation
- **WHEN** the user logs in with the new password
- **THEN** the new session carries the current credential version and validates successfully

#### Scenario: Legacy session value is fail-closed
- **GIVEN** a Redis session value uses the pre-versioned `v1` format
- **WHEN** the identity service validates that session
- **THEN** the value can be decoded for cleanup but does not pass credential-version validation

### Requirement: Atomic credential version persistence

The system SHALL update a user's password hash and credential version as one repository operation.

#### Scenario: Unknown or disabled user is not updated
- **WHEN** a password update targets an unknown or disabled user
- **THEN** the repository reports failure and neither credential value nor version is changed

### Requirement: Safe Redis session revocation scan

The system SHALL not issue string reads against Redis user-session index keys while scanning session keys for compatibility cleanup.

#### Scenario: Indexed and legacy sessions are revoked
- **WHEN** user-scoped revocation runs against Redis
- **THEN** indexed session keys and legacy unindexed session keys for that user are deleted
- **AND** sorted-set index keys are handled only as sorted sets

### Requirement: Distributed login-failure limiting

The system SHALL support a Redis-backed login-failure limiter that atomically shares per-username failure windows and lock state across backend instances.

#### Scenario: Failed attempts are shared across instances
- **GIVEN** two identity-service instances use the same Redis limiter namespace
- **WHEN** failed attempts for one username reach the configured threshold across both instances
- **THEN** subsequent login attempts are rejected as locked by either instance
- **AND** the response includes the remaining lock duration through `Retry-After`

#### Scenario: Successful login clears the failure state
- **GIVEN** a username has recorded failures but is not locked
- **WHEN** the username logs in successfully
- **THEN** the distributed failure state is cleared

#### Scenario: Limiter storage failure fails closed
- **GIVEN** Redis is selected for login rate limiting
- **WHEN** the limiter cannot read or update its state
- **THEN** login is rejected with `AUTHENTICATION_RATE_LIMITER_UNAVAILABLE` and HTTP `503`

### Requirement: Backward-compatible local limiter

The system SHALL retain an in-memory login limiter as the default when distributed limiting is not configured.

#### Scenario: Existing local lockout behavior remains available
- **GIVEN** `security.login_rate_limit_store` is `memory`
- **WHEN** failed attempts reach the configured threshold
- **THEN** the service returns the existing locked result without requiring Redis

### Requirement: Seed credentials require explicit governance

The system SHALL identify published seed accounts as requiring password rotation and SHALL reject ordinary sign-in for those accounts unless an explicit development compatibility setting is enabled.

#### Scenario: Production-oriented login rejects an unrotated seed account

- **GIVEN** a user credential requires password rotation
- **AND** `security.allow_seed_credentials` is disabled
- **WHEN** the user submits otherwise valid sign-in credentials
- **THEN** no session is issued
- **AND** the response code is `SEED_CREDENTIAL_ROTATION_REQUIRED` with HTTP `403`

#### Scenario: Local demo explicitly permits demonstration credentials

- **GIVEN** a user credential requires password rotation
- **AND** `security.allow_seed_credentials` is enabled
- **WHEN** the user submits valid credentials
- **THEN** normal authentication behavior remains available

### Requirement: Controlled seed credential rotation

The deployment tooling SHALL allow an operator with database access to rotate a named seed credential without recording the supplied password or resulting hash.

#### Scenario: Rotation clears the governance state

- **GIVEN** a seed user requires password rotation
- **WHEN** the deployment rotation utility persists a compliant replacement password
- **THEN** the stored credential uses a newly generated PBKDF2-SHA256 salt and hash
- **AND** the credential version is incremented
- **AND** the user no longer requires password rotation

#### Scenario: Existing non-seed credentials are not overwritten by migration

- **GIVEN** an existing user has a password hash different from the published seed hash
- **WHEN** the seed governance migration runs
- **THEN** its password hash and password-rotation state are unchanged
