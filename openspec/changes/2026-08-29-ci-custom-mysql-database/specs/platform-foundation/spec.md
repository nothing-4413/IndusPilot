# Change: CI 自定义 MySQL 数据库验证

## ADDED Requirements

### Requirement: CI 必须验证非默认 MySQL 数据库名
系统 SHALL 在 dependency-services 和 real backend runtime profile CI job 中使用同一个非默认的 MySQL 数据库名。

#### Scenario: 依赖服务使用自定义数据库
- **WHEN** dependency-services CI job 创建临时部署环境
- **THEN** `INDUSPILOT_MYSQL_DATABASE` SHALL 被设置为 `induspilot_ci_custom_db`
- **AND** 依赖 smoke SHALL 验证容器中的实际数据库名

#### Scenario: 真实运行时 profile 保持名称一致
- **WHEN** backend runtime profile CI job 启动 MySQL 并运行 HTTP smoke
- **THEN** migration、CRUD smoke 和后端 MySQL 配置 SHALL 使用 `induspilot_ci_custom_db`
- **AND** HTTP smoke SHALL 收到显式的数据库名参数
