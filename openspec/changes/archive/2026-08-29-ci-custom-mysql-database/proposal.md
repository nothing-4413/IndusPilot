# Change: CI 自定义 MySQL 数据库验证

## Why

本地和部署脚本已经支持可配置 MySQL 数据库名，但 CI 一直保留默认 `induspilot`。这不能证明 Compose、迁移、依赖 CRUD 和后端真实运行时会在同一个非默认数据库上工作。

## What Changes

- CI 依赖服务和真实运行时 profile 均使用 `induspilot_ci_custom_db`。
- 依赖 smoke 可选择性断言 Compose 容器实际数据库名。
- 运行时 profile 将数据库名显式传给 HTTP smoke。
- 部署预检静态校验两条 CI 链路都保留自定义数据库覆盖。

## Non-Goals

- 不更改生产默认数据库名。
- 不改变 MySQL migration 或 schema 语义。
