## ADDED Requirements

### Requirement: MySQL 健康探测遵循连接 URI 优先级

系统 SHALL 在 MySQL 健康探测中使用与 MySQL 仓储一致的连接优先级：`mysql.uri` 非空时优先解析 URI，URI 为空时使用 `mysql.host` 和 `mysql.port`。

#### Scenario: 配置了 MySQL URI

- **GIVEN** `mysql.uri` 非空
- **WHEN** 后端生成依赖健康状态
- **THEN** MySQL TCP 探测 SHALL use the host and port parsed from `mysql.uri`

#### Scenario: 未配置 MySQL URI

- **GIVEN** `mysql.uri` 为空
- **WHEN** 后端生成依赖健康状态
- **THEN** MySQL TCP 探测 SHALL use `mysql.host` and `mysql.port`