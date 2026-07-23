## ADDED Requirements

### Requirement: Qt 客户端告警创建

Qt 客户端 SHALL provide an authenticated workflow for operators to create alerts through the backend.

#### Scenario: 创建告警后刷新列表

- **GIVEN** 操作员已通过 Qt 客户端登录后端
- **WHEN** 操作员填写告警编号、设备编号、级别、状态、标题和负责人并提交
- **THEN** 客户端 SHALL call `POST /api/v1/alerts`
- **AND** 客户端 SHALL refresh the alert table after a successful response
- **AND** 客户端 SHALL display the latest operation result from the API client

#### Scenario: 必填字段缺失时阻止创建

- **GIVEN** 操作员未填写告警编号、设备编号、级别或标题
- **WHEN** 操作员提交创建告警
- **THEN** 客户端 SHALL not treat the operation as successful
- **AND** 客户端 SHALL show a Chinese message describing the missing condition
