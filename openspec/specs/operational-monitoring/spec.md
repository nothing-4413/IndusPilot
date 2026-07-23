# operational-monitoring Specification

## Purpose
定义设备运行状态写入、单设备查询、状态列表和状态/严重度汇总能力，为告警触发和运维决策提供实时运行上下文。
## Requirements
### Requirement: Equipment status overview
The system SHALL provide an overview of equipment operational status for dashboard and list views.

#### Scenario: Operator opens monitoring dashboard
- **WHEN** an operator opens the monitoring dashboard
- **THEN** the system displays summarized equipment status grouped by operational state and severity

### Requirement: Runtime state separation
The system SHALL distinguish durable asset metadata from short-lived runtime state.

#### Scenario: Runtime state changes
- **WHEN** equipment runtime state changes
- **THEN** the system updates the monitoring state without altering durable asset identity metadata

### Requirement: Real-time update channel
The system SHALL define a real-time update channel for monitoring-facing state changes.

#### Scenario: Equipment state update is pushed
- **WHEN** a monitored equipment state changes
- **THEN** connected clients receive an update without requiring a full manual refresh

### Requirement: Runtime state HTTP API
The system SHALL expose authenticated HTTP endpoints for listing, retrieving, and updating short-lived equipment runtime state.

#### Scenario: Authorized user lists runtime states
- **WHEN** a user with `asset:read` opens the monitoring state list
- **THEN** the system returns current runtime states and a summary grouped by state

#### Scenario: Authorized user retrieves runtime state for one asset
- **WHEN** a user with `asset:read` requests runtime state by asset identifier
- **THEN** the system returns the current runtime state or a not-found response

#### Scenario: Authorized user updates runtime state
- **WHEN** a user with `monitoring:write` submits a runtime state payload
- **THEN** the system updates the runtime state without modifying durable asset metadata

### Requirement: Monitoring write permission
The system SHALL distinguish monitoring runtime-state write permission from asset master-data write permission.

#### Scenario: User without monitoring write permission updates runtime state
- **WHEN** a user has `asset:read` but lacks `monitoring:write`
- **THEN** the system rejects the runtime-state update with an authorization error

