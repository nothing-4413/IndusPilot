# Change: 运行模块持久化仓储

## Why

当前 `storage.repository_store=mysql` 已经能驱动身份和资产模块，但告警、工单、运行监控和 AI 交互审计仍保存在进程内存中。服务重启后运行数据丢失，也会让配置项的生产化语义不完整。为了让工业支持系统更接近可部署形态，需要把核心运行模块接入统一仓储边界，并在 MySQL 模式下使用数据库持久化。

## What Changes

- 为运行监控状态补充仓储接口和内存/MySQL 实现。
- 为告警、工单和 AI 交互审计补齐 MySQL 仓储实现。
- 让 HTTP 运行时在 `repository_store=mysql` 时注入运行模块 MySQL 仓储，默认仍使用内存仓储。
- 补充 MySQL schema、配置文档和测试，证明默认模式保持自包含，MySQL 模式具备可落库边界。

## Non-Goals

- 不在本次实现异步事件队列或 Redis runtime state cache。
- 不在本次实现真实外部 AI 推理调用。
- 不在本次实现数据库迁移工具，只提供可执行 SQL schema 增量。