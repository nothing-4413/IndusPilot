# Change: 增加真实数据库 CRUD 集成测试

## Why

现有 dependency smoke 已能启动 MySQL、Redis、MongoDB 并验证最小连通性，但 MySQL 验证仍偏向迁移脚本和表结构检查。为了更接近生产级质量门禁，需要在真实 MySQL 容器中执行可重复的业务 CRUD 链路，覆盖核心工业运维对象、关联关系和操作审计写入。

## What Changes

- 增加 `database/mysql/integration/real_crud_smoke.sql`，在真实 MySQL 中执行可重复的自断言 CRUD smoke。
- 扩展 `backend/tests/dependency_services_smoke.sh`，在迁移幂等验证后执行真实 CRUD smoke。
- CRUD smoke 覆盖默认用户、资产、运行状态、告警规则、告警、通知投递、工单、附件、AI 交互和操作审计事件。
- 更新开发、部署和生产就绪文档，明确 dependency-services CI 不再只是连接检查。

## Non-Goals

- 本次不引入后端服务连接真实 MySQL 的端到端 HTTP 测试。
- 本次不引入 Redis 会话写读集成测试或 MongoDB 文档 CRUD 测试。