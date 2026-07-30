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

