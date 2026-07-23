## ADDED Requirements

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