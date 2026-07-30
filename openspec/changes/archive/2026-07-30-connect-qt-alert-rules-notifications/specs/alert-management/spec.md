## ADDED Requirements

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