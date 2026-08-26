# Change: 增加生产 readiness 与启动策略

## Why

当前后端启动只执行一次无条件依赖 TCP 探测，探测失败仍将进程标记为运行；`/health` 也无法区分进程存活、依赖就绪和启动配置错误。memory 模式还会探测未使用的外部依赖，导致本地运行和生产编排的语义不一致。

## What Changes

- 增加启动配置校验，将静态配置错误与依赖暂时不可用区分开。
- 根据 `repository_store`、`session_store` 和 AI 配置计算 required/optional 依赖集合。
- 增加 liveness、readiness、startup 三类平台检查，同时保持 `/health` 旧接口可用。
- 为依赖探测增加有限超时、缓存和恢复后的重新评估能力。
- 更新 HTTP smoke、部署 healthcheck、开发和生产运行文档。

## Non-Goals

- 本次不实现数据库 schema 自动迁移。
- 本次不把 MongoDB 接入业务仓储；MongoDB 继续作为可选能力。
- 本次不要求 AI provider 成为核心服务启动条件；只有显式配置 required 时才阻断 readiness。
- 本次不改变业务 API、认证模型或仓储协议。
