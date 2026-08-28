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
The system SHALL support session creation, validation, expiration, and logout.

#### Scenario: Expired session is rejected
- **WHEN** a user sends a request with an expired session
- **THEN** the system rejects the request and requires re-authentication

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
