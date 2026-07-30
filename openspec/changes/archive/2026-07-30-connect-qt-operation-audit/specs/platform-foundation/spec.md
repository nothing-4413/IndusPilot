## ADDED Requirements

### Requirement: 操作审计事件仓储

系统 SHALL 使用统一仓储保存关键业务操作审计事件，事件至少包含编号、操作者、动作、资源类型、资源编号、结果、追踪号和发生时间。

#### Scenario: 审计服务记录事件

- **WHEN** 后端服务提交一条缺少编号或发生时间的操作审计事件
- **THEN** 系统 SHALL 自动补齐审计编号和发生时间
- **AND** 事件 SHALL 写入当前配置的内存或 MySQL 仓储

### Requirement: 操作审计 HTTP 查询

系统 SHALL 提供受权限保护的操作审计查询接口。

#### Scenario: 管理员查询操作审计

- **GIVEN** 管理员已登录并持有 `audit:read` 权限
- **WHEN** 管理员调用 `GET /api/v1/audit/events`
- **THEN** 后端 SHALL 返回最近的操作审计事件数组
- **AND** 每条记录 SHALL 包含操作者、动作、资源、结果、追踪号和发生时间

#### Scenario: 非授权用户被拒绝

- **GIVEN** 操作员已登录但没有 `audit:read` 权限
- **WHEN** 操作员调用 `GET /api/v1/audit/events`
- **THEN** 后端 SHALL 返回 HTTP 403

### Requirement: 关键操作自动审计

系统 SHALL 对登录成功、告警通知派发和告警通知重试写入操作审计事件。

#### Scenario: 登录成功写入审计

- **WHEN** 用户使用有效凭据登录成功
- **THEN** 后端 SHALL 写入 `auth.login` 操作审计事件

#### Scenario: 告警通知派发写入审计

- **WHEN** 操作员派发待投递告警通知
- **THEN** 后端 SHALL 写入 `alert-notification.dispatch` 操作审计事件
- **AND** 审计资源编号 SHALL 包含成功、失败和跳过数量摘要

### Requirement: Qt 客户端操作审计页面

Qt 客户端 SHALL 在管理员登录后展示操作审计事件。

#### Scenario: 客户端同步操作审计

- **WHEN** Qt 客户端已登录并刷新在线数据
- **THEN** 客户端 SHALL 调用 `GET /api/v1/audit/events`
- **AND** “操作审计”页面 SHALL 展示编号、用户、动作、资源类型、资源编号、结果、追踪和时间

#### Scenario: 审计查询失败时展示离线兜底

- **WHEN** 后端不可用、未登录或审计查询失败
- **THEN** Qt 客户端 SHALL 保留本地演示审计记录
- **AND** 客户端 SHALL 使用中文状态消息说明当前为离线兜底数据