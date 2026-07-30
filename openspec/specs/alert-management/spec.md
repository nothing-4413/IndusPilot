# alert-management Specification

## Purpose
定义告警记录、严重度、确认、分派、解决和关闭生命周期，用于支撑工业现场异常事件从发现到闭环处置的完整流程。
## Requirements
### Requirement: Alert record lifecycle
The system SHALL manage alert records from creation through acknowledgement, assignment, resolution, and closure.

#### Scenario: Alert is acknowledged
- **WHEN** an operator acknowledges an open alert
- **THEN** the system records the acknowledgement user and timestamp

### Requirement: Alert severity
The system SHALL classify alerts by severity and expose severity in list, detail, dashboard, and notification contexts.

#### Scenario: Critical alert appears
- **WHEN** a critical alert is created
- **THEN** the system marks it with critical severity and prioritizes it in alert views

### Requirement: Alert relationship to assets
The system SHALL associate alerts with equipment assets when the source equipment is known.

#### Scenario: Asset alert is viewed
- **WHEN** a user opens an alert linked to equipment
- **THEN** the system shows the related asset context and navigation path

### Requirement: Alert HTTP API
The system SHALL expose authenticated HTTP endpoints for creating, listing, retrieving, and transitioning alert records.

#### Scenario: Authorized user creates an alert
- **WHEN** a user with `alert:write` submits a valid alert payload
- **THEN** the system stores the alert and returns the created record with severity and state

#### Scenario: Authorized user transitions alert lifecycle
- **WHEN** a user with `alert:write` acknowledges, assigns, resolves, or closes an alert
- **THEN** the system stores the new alert lifecycle state and related operator fields

#### Scenario: Authorized user retrieves alert detail
- **WHEN** a user with `alert:read` requests an alert by identifier
- **THEN** the system returns the alert record or a not-found response

### Requirement: Alert filtering
The system SHALL support filtering alert records by related asset, severity, and lifecycle state.

#### Scenario: User filters critical open alerts
- **WHEN** a user lists alerts with severity and state filters
- **THEN** the system returns only matching alert records

### Requirement: 告警规则配置

系统 SHALL allow authorized users to define alert fanout rules that match alerts by minimum severity, optional asset, channel, target, and enabled state.

#### Scenario: 创建告警规则

- **WHEN** 具备 `alert:write` 权限的用户提交规则编号、名称、最低级别、通道和目标
- **THEN** 后端 SHALL persist the alert rule
- **AND** Qt 客户端 SHALL provide a Chinese rule creation dialog from the alert center

#### Scenario: 查询告警规则

- **WHEN** 已认证用户查看告警规则
- **THEN** 后端 SHALL return configured rules with enabled state
- **AND** Qt 客户端 SHALL display rule rows in the alert center workflow

### Requirement: 告警通知记录

系统 SHALL create auditable notification records when a new alert matches enabled alert rules.

#### Scenario: 告警命中规则生成通知

- **GIVEN** an enabled alert rule targets the alert asset or has no asset constraint
- **WHEN** a new alert is created with severity greater than or equal to the rule minimum severity
- **THEN** 后端 SHALL create a queued notification record linked to the alert and rule
- **AND** the notification SHALL expose channel, target, status, and message for audit

#### Scenario: 查询告警通知

- **WHEN** 已认证用户查看告警通知
- **THEN** 后端 SHALL return generated notification records
- **AND** Qt 客户端 SHALL show the notification rows in a Chinese table

### Requirement: 告警通知投递状态流转

系统 SHALL maintain auditable delivery state for alert notification records.

#### Scenario: 投递队列通知

- **GIVEN** queued alert notification records exist
- **WHEN** 具备 `alert:write` 权限的用户触发通知队列投递
- **THEN** 后端 SHALL attempt supported-channel delivery for queued notifications
- **AND** 后端 SHALL update status, attempt count, failure reason, and delivered time according to the result

#### Scenario: 重试单条通知

- **WHEN** 具备 `alert:write` 权限的用户重试一条未成功通知
- **THEN** 后端 SHALL mark it as retrying and perform another delivery attempt
- **AND** Qt 客户端 SHALL refresh the notification table after retry

#### Scenario: 查询投递审计字段

- **WHEN** 已认证用户查看告警通知记录
- **THEN** 后端 SHALL return status, attempt count, last error, delivered time, channel, target, and message
- **AND** Qt 客户端 SHALL display these fields in Chinese table headers

### Requirement: Alert persistence repository
The system SHALL persist alert lifecycle records through an injectable alert repository.

#### Scenario: Alert lifecycle is saved through repository
- **WHEN** an alert is created, acknowledged, assigned, resolved, or closed
- **THEN** the updated alert record is saved through the configured repository
