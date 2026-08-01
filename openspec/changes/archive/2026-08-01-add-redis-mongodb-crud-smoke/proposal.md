# Change: 增加 Redis/MongoDB 真实 CRUD 集成测试

## Why

dependency-services CI 已经覆盖 MySQL 真实 CRUD，但 Redis 和 MongoDB 仍主要停留在认证 PING 与初始化脚本加载。为了提升依赖验证的生产可信度，需要让 CI 对缓存键值、过期时间、哈希结构以及 MongoDB 文档集合、索引和文档 upsert 做真实读写断言。

## What Changes

- 扩展 `backend/tests/dependency_services_smoke.sh`，增加 Redis key/value、TTL、counter 和 hash 的真实读写断言。
- 增加 `database/mongodb/integration/real_crud_smoke.js`，覆盖 MongoDB collection、索引和文档 upsert/read 验证。
- 调整 MongoDB compose 挂载为目录挂载，使初始化脚本和 integration smoke 脚本都能被容器访问。
- 扩展部署预检和文档，确保真实依赖 CRUD smoke 成为仓库级质量门禁的一部分。

## Non-Goals

- 本次不把 MongoDB 接入后端业务仓储。
- 本次不实现 Redis-backed session 的 HTTP 端到端生命周期测试。