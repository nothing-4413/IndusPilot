## ADDED Requirements

### Requirement: Qt 客户端资产状态更新

Qt 客户端 SHALL provide an authenticated workflow for operators to update an equipment asset lifecycle status.

#### Scenario: 选中资产后更新状态

- **GIVEN** 操作员已通过 Qt 客户端登录后端
- **AND** 资产页面存在一条选中的资产
- **WHEN** 操作员选择新的资产状态并提交
- **THEN** 客户端 SHALL call `PATCH /api/v1/assets/{id}/status`
- **AND** 客户端 SHALL refresh the asset table after a successful response
- **AND** 客户端 SHALL display the latest operation result from the API client

#### Scenario: 未选择资产时阻止更新

- **GIVEN** 操作员未选择资产
- **WHEN** 操作员提交状态更新
- **THEN** 客户端 SHALL not treat the operation as successful
- **AND** 客户端 SHALL show a Chinese message describing the missing selection
