## ADDED Requirements

### Requirement: 操作审计筛选查询

系统 SHALL 支持管理员按关键字段筛选操作审计事件。

#### Scenario: 按操作者和动作筛选

- **GIVEN** 管理员已登录并持有 `audit:read` 权限
- **WHEN** 管理员调用 `GET /api/v1/audit/events?actor=operator&action=alert-notification.dispatch`
- **THEN** 后端 SHALL 仅返回匹配操作者和动作的操作审计事件

#### Scenario: 按资源类型和结果筛选

- **GIVEN** 已存在告警通知派发审计事件
- **WHEN** 管理员调用 `GET /api/v1/audit/events?resourceType=alert-notification-batch&result=success`
- **THEN** 后端 SHALL 仅返回匹配资源类型和结果的审计事件

### Requirement: 操作审计分页查询

系统 SHALL 在审计查询传入分页参数时返回分页对象，并在未传分页参数时保持原数组响应兼容性。

#### Scenario: 分页查询返回总数和当前页

- **WHEN** 管理员调用 `GET /api/v1/audit/events?limit=20&offset=0`
- **THEN** 后端 SHALL 返回对象格式的 `data` 字段
- **AND** `data.items` SHALL contain the current page of operation audit events
- **AND** `data.total`, `data.limit`, and `data.offset` SHALL describe the full filtered result set and page request

#### Scenario: 无分页参数保持数组响应

- **WHEN** 管理员调用 `GET /api/v1/audit/events` 且未传入 `limit` 或 `offset`
- **THEN** 后端 SHALL 返回数组格式的 `data` 字段

#### Scenario: 无效分页参数被拒绝

- **WHEN** 管理员传入小于 1 或大于 100 的 `limit`，或传入负数 `offset`
- **THEN** 后端 SHALL return HTTP 400 with a Chinese validation message

### Requirement: Qt 客户端操作审计筛选分页控件

Qt 客户端 SHALL allow administrators to filter and page operation audit events from the desktop UI.

#### Scenario: 管理员筛选操作审计

- **GIVEN** “操作审计”页面已经打开
- **WHEN** 管理员填写用户、动作、资源类型或结果条件并点击刷新
- **THEN** Qt 客户端 SHALL request `GET /api/v1/audit/events` with the selected filters and pagination parameters
- **AND** 审计表格 SHALL display only the returned page

#### Scenario: 管理员翻页查看审计记录

- **GIVEN** 当前筛选条件存在多页审计记录
- **WHEN** 管理员点击上一页或下一页
- **THEN** Qt 客户端 SHALL update `offset` and refresh the audit table
- **AND** 页面提示 SHALL show the visible range and total count in Chinese