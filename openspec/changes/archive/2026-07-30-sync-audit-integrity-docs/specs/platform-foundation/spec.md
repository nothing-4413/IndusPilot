## ADDED Requirements

### Requirement: 开发文档描述当前审计完整性能力

开发文档 SHALL 描述当前操作审计查询、CSV 导出和哈希链完整性校验接口，且不得把已实现的依赖探测描述为旧版临时实现。

#### Scenario: 开发者查看后端 HTTP 文档

- **GIVEN** 开发者打开后端 HTTP 服务文档
- **WHEN** 开发者查看健康检查与操作审计章节
- **THEN** 文档 SHALL describe dependency connectivity probing
- **AND** 文档 SHALL include `GET /api/v1/audit/integrity`
- **AND** 文档 SHALL describe audit hash-chain fields and verification result

### Requirement: 示例配置说明运行时优先级

示例配置 SHALL 明确 MySQL `uri` 非空时优先于 host/port/database/user/password 组合连接参数。

#### Scenario: 开发者配置 MySQL URI

- **GIVEN** 开发者打开 `config/backend.example.yaml`
- **WHEN** 开发者查看 `mysql.uri` 注释
- **THEN** 注释 SHALL explain that non-empty `uri` is used first by the MySQL repository