## ADDED Requirements

### Requirement: AI 交互审计分页查询

系统 SHALL 支持对 AI 交互审计记录进行兼容式分页查询。

#### Scenario: 旧版审计查询保持数组响应

- **WHEN** 已认证用户调用 `GET /api/v1/ai/interactions` 且未传入 `limit` 或 `offset`
- **THEN** 后端 SHALL 返回既有数组格式的 `data` 字段
- **AND** 既有 Qt 客户端和脚本 SHALL 不需要修改即可继续读取审计记录

#### Scenario: 分页审计查询返回总数和当前页

- **WHEN** 已认证用户调用 `GET /api/v1/ai/interactions?limit=10&offset=0`
- **THEN** 后端 SHALL 返回对象格式的 `data` 字段
- **AND** `data.items` SHALL contain the current page of AI interaction records
- **AND** `data.total`, `data.limit`, and `data.offset` SHALL describe the full filtered result set and page request

#### Scenario: 无效分页参数被拒绝

- **WHEN** 已认证用户传入小于 1 或大于 100 的 `limit`，或传入负数 `offset`
- **THEN** 后端 SHALL return HTTP 400 with a Chinese validation message

### Requirement: Qt 客户端 AI 审计分页控件

Qt 客户端 SHALL allow operators to browse AI interaction audit records by page.

#### Scenario: 操作员翻页查看审计

- **GIVEN** AI 辅助页面已经打开
- **WHEN** 操作员点击上一页或下一页
- **THEN** Qt 客户端 SHALL request the backend with the selected `limit` and calculated `offset`
- **AND** the audit table SHALL display only the returned page
- **AND** the page summary SHALL show the visible range and total count in Chinese