## ADDED Requirements

### Requirement: Qt 客户端运行状态写入

Qt 客户端 SHALL provide an authenticated workflow for operators to submit runtime monitoring states to the backend.

#### Scenario: 写入运行状态后刷新列表

- **GIVEN** 操作员已通过 Qt 客户端登录后端
- **WHEN** 操作员填写设备编号、运行状态、严重度和指标摘要并提交
- **THEN** 客户端 SHALL call `POST /api/v1/monitoring/states`
- **AND** 客户端 SHALL refresh the runtime monitoring table after a successful response
- **AND** 客户端 SHALL display the latest operation result from the API client

#### Scenario: 未登录或字段不完整时阻止写入

- **GIVEN** 操作员未登录后端或未填写必需字段
- **WHEN** 操作员提交运行状态
- **THEN** 客户端 SHALL not treat the operation as successful
- **AND** 客户端 SHALL show a Chinese message describing the missing condition
