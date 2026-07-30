## ADDED Requirements

### Requirement: 操作审计导出权限

系统 SHALL 使用独立权限控制操作审计导出能力。

#### Scenario: 无导出权限用户被拒绝

- **GIVEN** 操作员已登录但没有 `audit:export` 权限
- **WHEN** 操作员调用 `GET /api/v1/audit/events/export`
- **THEN** 后端 SHALL 返回 HTTP 403

#### Scenario: 管理员具备默认导出权限

- **GIVEN** 默认管理员角色已经初始化
- **WHEN** 管理员登录后请求导出操作审计
- **THEN** 后端 SHALL allow the export request

### Requirement: 操作审计 CSV 导出接口

系统 SHALL 支持按当前筛选条件导出操作审计 CSV。

#### Scenario: 管理员导出筛选后的审计记录

- **GIVEN** 管理员已登录并持有 `audit:export` 权限
- **WHEN** 管理员调用 `GET /api/v1/audit/events/export?actor=admin&action=auth.login`
- **THEN** 后端 SHALL return HTTP 200 with `text/csv` content
- **AND** CSV SHALL contain the header `id,actor,action,resourceType,resourceId,result,traceId,occurredAt`
- **AND** CSV SHALL contain only records matching the supplied filters

#### Scenario: 导出操作自身被审计

- **WHEN** 管理员成功导出操作审计 CSV
- **THEN** 后端 SHALL record an `operation-audit.export` audit event
- **AND** the event resource id SHALL include the exported record count

### Requirement: Qt 客户端操作审计导出

Qt 客户端 SHALL allow administrators to export operation audit records from the current filter context.

#### Scenario: 客户端导出当前筛选审计

- **GIVEN** “操作审计”页面已经打开
- **WHEN** 管理员点击导出
- **THEN** Qt 客户端 SHALL call `GET /api/v1/audit/events/export` with the selected filters
- **AND** 客户端 SHALL save the returned CSV to a user-selected local path

#### Scenario: 导出失败时展示中文提示

- **WHEN** 后端拒绝导出或请求失败
- **THEN** Qt 客户端 SHALL show a Chinese failure message and SHALL NOT create an empty export file