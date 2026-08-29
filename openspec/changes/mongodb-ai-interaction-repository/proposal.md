# Change: MongoDB AI 交互仓储

## Why

MongoDB 已在 compose、初始化脚本和 CI 中验证可用，但后端仍将 AI 交互记录固定写入内存或 MySQL。AI 交互和诊断上下文是文档型数据，接入现有 MongoDB 集合可以降低事务库压力，并让已验证的 Mongo 能力进入实际运行时。

## What Changes

- 增加可注入的 MongoDB AI 交互仓储实现，支持 upsert 和按关联对象查询。
- 增加 `storage.ai_interaction_store` 配置，独立选择 `memory`、`mysql` 或 `mongodb`。
- 在 Drogon HTTP runtime 中按配置创建 MongoDB client，并在 Mongo 不可用时让启动/业务错误保持可诊断。
- 更新依赖声明、CI runtime profile、部署文档和测试覆盖。

## Non-goals

- 不迁移身份、资产、告警、工单或操作审计哈希链。
- 不在本变更中引入跨 MySQL/MongoDB 事务。
- 不改变 AI HTTP API 的响应格式和现有内存/MySQL 兼容路径。
