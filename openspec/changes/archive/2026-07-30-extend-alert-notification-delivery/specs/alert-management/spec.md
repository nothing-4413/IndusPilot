## ADDED Requirements

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