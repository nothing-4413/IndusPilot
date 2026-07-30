## ADDED Requirements

### Requirement: Qt 客户端 AI 审计导出

Qt 客户端 SHALL allow operators to export the currently displayed AI interaction audit table as a CSV file.

#### Scenario: 导出当前审计表格

- **GIVEN** AI 交互审计表格中存在记录
- **WHEN** 操作员选择导出审计并确认保存路径
- **THEN** 客户端 SHALL write a UTF-8 CSV file containing the table headers and rows
- **AND** 客户端 SHALL escape CSV fields containing commas, quotes or new lines
- **AND** 客户端 SHALL show a Chinese success message containing the saved path

#### Scenario: 审计表为空时阻止导出

- **GIVEN** AI 交互审计表格中没有记录
- **WHEN** 操作员选择导出审计
- **THEN** 客户端 SHALL not create a file
- **AND** 客户端 SHALL show a Chinese message asking the operator to refresh or query audit records first
