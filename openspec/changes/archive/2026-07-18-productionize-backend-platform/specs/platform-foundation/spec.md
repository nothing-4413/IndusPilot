## ADDED Requirements

### Requirement: Backend service runtime

The system SHALL expose a configurable backend service runtime instead of relying only on in-process module calls.

#### Scenario: Backend starts as a service
- **WHEN** the backend starts with valid configuration
- **THEN** it binds to the configured host and port
- **AND** exposes health and module API endpoints

#### Scenario: Backend reports dependency health
- **WHEN** a health request is received
- **THEN** the response includes MySQL, Redis, MongoDB, and AI adapter availability

### Requirement: Configuration management

The system SHALL load backend configuration from structured files with environment variable overrides for deployable environments.

#### Scenario: Environment overrides service configuration
- **WHEN** an environment variable overrides a configured dependency value
- **THEN** the backend uses the environment value during startup and health checks

### Requirement: Protected API operations

The system SHALL require valid sessions and permissions before protected industrial operations can be executed.

#### Scenario: Protected route rejects missing session
- **WHEN** a request to a protected route omits a valid session token
- **THEN** the backend returns an authentication error without executing the operation

#### Scenario: Protected route rejects missing permission
- **WHEN** an authenticated user lacks the permission required by a route
- **THEN** the backend returns an authorization error without executing the operation