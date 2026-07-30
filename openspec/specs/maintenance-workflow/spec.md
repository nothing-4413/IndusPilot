# maintenance-workflow Specification

## Purpose
定义维护工单创建、从告警转工单、分派、开始处理、完成、关闭以及资产维护历史查询能力，用于支撑维修闭环。
## Requirements
### Requirement: Work order lifecycle
The system SHALL manage maintenance work orders through creation, assignment, processing, completion, and closure.

#### Scenario: Work order is assigned
- **WHEN** a dispatcher assigns a work order to a maintainer
- **THEN** the system records the assignee and updates the work order state

### Requirement: Work order relationship to alerts and assets
The system SHALL allow work orders to reference related alerts and equipment assets.

#### Scenario: Work order is created from alert
- **WHEN** an operator creates a work order from an alert
- **THEN** the system links the work order to the source alert and equipment asset

### Requirement: Maintenance history
The system SHALL preserve completed maintenance records for future asset review and AI-assisted diagnosis.

#### Scenario: Asset maintenance history is reviewed
- **WHEN** a user opens an asset maintenance history
- **THEN** the system lists related closed work orders and maintenance outcomes

### Requirement: Work order HTTP API
The system SHALL expose authenticated HTTP endpoints for creating, listing, retrieving, and transitioning maintenance work orders.

#### Scenario: Authorized user creates a work order
- **WHEN** a user with `work-order:write` submits a valid work order payload
- **THEN** the system stores the work order and returns the created record

#### Scenario: Authorized user transitions work order lifecycle
- **WHEN** a user with `work-order:write` assigns, starts, completes, or closes a work order
- **THEN** the system stores the new work order state and related execution fields

### Requirement: Work order creation from alert API
The system SHALL allow authorized users to create work orders from existing alerts through HTTP.

#### Scenario: User creates work order from alert
- **WHEN** a user with `work-order:write` submits an existing alert identifier and summary
- **THEN** the system creates a work order linked to the source alert and asset

### Requirement: Asset maintenance history API
The system SHALL expose completed and closed maintenance history for an equipment asset.

#### Scenario: User views asset maintenance history
- **WHEN** a user with `work-order:read` requests maintenance history for an asset
- **THEN** the system returns closed work orders linked to that asset

### Requirement: 工单编辑

系统 SHALL allow authorized maintainers to edit mutable work order fields after creation.

#### Scenario: 编辑维护工单

- **WHEN** 具备 `work-order:write` 权限的用户提交工单摘要、处理人或处理结果更新
- **THEN** 后端 SHALL update the selected work order and return the updated record
- **AND** Qt 客户端 SHALL refresh the work order table after a successful edit

### Requirement: 工单附件元数据

系统 SHALL allow operators to register and view attachment metadata for a work order.

#### Scenario: 登记附件元数据

- **WHEN** 具备 `work-order:write` 权限的用户为工单提交附件编号、文件名、URI、类型和大小
- **THEN** 后端 SHALL persist the attachment metadata linked to the work order
- **AND** 后端 SHALL default the uploader to the current session user when not supplied

#### Scenario: 查询工单附件

- **WHEN** 已认证用户查看工单附件
- **THEN** 后端 SHALL return all attachment metadata linked to the selected work order
- **AND** Qt 客户端 SHALL display the attachment rows in Chinese table headers

### Requirement: Work order persistence repository
The system SHALL persist work order lifecycle records through an injectable work order repository.

#### Scenario: Work order lifecycle is saved through repository
- **WHEN** a work order is created, assigned, started, completed, or closed
- **THEN** the updated work order record is saved through the configured repository
