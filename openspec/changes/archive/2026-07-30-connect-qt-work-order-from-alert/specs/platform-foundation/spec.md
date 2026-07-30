## ADDED Requirements

### Requirement: Qt 客户端从告警生成工单

Qt 客户端 SHALL provide an authenticated workflow for operators to create a maintenance work order from a selected alert.

#### Scenario: 选中告警后生成工单

- **GIVEN** 操作员已通过 Qt 客户端登录后端
- **AND** 告警中心存在一条选中的告警
- **WHEN** 操作员填写处置摘要并提交生成工单
- **THEN** 客户端 SHALL call `POST /api/v1/work-orders/from-alert` with the selected `alertId`
- **AND** 客户端 SHALL refresh the work order table after a successful response
- **AND** 客户端 SHALL display the latest operation result from the API client

#### Scenario: 未选择告警时阻止生成

- **GIVEN** 操作员未选择告警
- **WHEN** 操作员提交生成工单
- **THEN** 客户端 SHALL not treat the operation as successful
- **AND** 客户端 SHALL show a Chinese message describing the missing selection
