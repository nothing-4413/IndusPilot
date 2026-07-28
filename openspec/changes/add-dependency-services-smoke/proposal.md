# Change: 增加真实依赖集成冒烟测试

## Why

当前 CI 已覆盖后端基础构建、OpenSpec 和配置预检，但没有真正启动 MySQL、Redis、MongoDB。数据库迁移、Redis 鉴权和 MongoDB 初始化脚本如果只停留在静态检查，仍可能在真实部署时失败。工业支持系统需要证明核心依赖编排能被自动拉起并完成最小可用验证。

## What Changes

- 新增 `backend/tests/dependency_services_smoke.sh`，复用 `deployment/docker-compose.yml` 验证真实依赖。
- GitHub Actions 新增 `dependency services` job，创建 CI `.env`、启动 compose、运行冒烟测试并清理卷。
- 冒烟测试重复执行 MySQL 迁移脚本，验证 `schema_migrations` 和审计哈希列。
- 冒烟测试验证 Redis 密码连接和 MongoDB 初始化脚本加载。
- 部署文档补充本地运行方式。

## Non-Goals

- 不在本模块构建 Drogon 后端并接入真实依赖仓储。
- 不引入长时间性能测试或压力测试。
- 不替代后续 repository 级集成测试。