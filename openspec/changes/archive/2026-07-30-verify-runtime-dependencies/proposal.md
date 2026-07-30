# Change: 运行依赖预检与 schema 版本基线

## Why

当前 `/health` 只做 TCP 连通性探测，部署文档也要求人工执行 MySQL 脚本。随着 MySQL 仓储、Redis 会话和 MongoDB 预留能力逐步接入，项目需要一个更清晰的部署前检查基线，避免配置、脚本和真实依赖状态不一致。

## What Changes

- 在 MySQL 初始化脚本中增加 `schema_migrations` 版本表，并登记已执行脚本版本。
- 新增部署前预检脚本，检查配置文件、数据库脚本、schema 版本登记、docker compose 服务和关键配置边界。
- 更新部署文档和生产就绪清单，明确 TCP health 与部署前预检的职责差异。

## Non-Goals

- 不在本次实现后端启动时自动迁移数据库。
- 不在本次要求预检脚本必须连接真实 MySQL 并执行 SQL 查询。
- 不替代后续 CI 中的真实依赖集成测试。