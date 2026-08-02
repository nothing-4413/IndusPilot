# Change: 增加 HTTP 真实运行时 profile 验收脚本

## Why

HTTP smoke 已支持切换 MySQL 仓储和 Redis session，但仍需要开发者手动拼接多个连接参数，容易出现示例密钥误用或连接字符串写错。为了把真实运行时验收变成可复现流程，需要提供一个一键脚本从 `deployment/.env` 读取依赖配置，组合连接参数，并复用已有 dependency smoke 与 HTTP smoke。

## What Changes

- 增加 `backend/tests/http_runtime_profile_smoke.ps1`，封装真实依赖 profile 验收流程。
- 脚本从 `deployment/.env` 读取 MySQL、Redis、MongoDB 连接信息，拒绝 `change-me-*` 示例密钥。
- 脚本可选启动 compose、运行 dependency CRUD smoke、执行 MySQL 仓储 + Redis session 的 HTTP smoke，并可选清理 compose。
- 部署预检纳入该脚本，文档增加运行入口。

## Non-Goals

- 本次不强制 CI 运行 Drogon/vcpkg 真实仓储 HTTP profile。
- 本次不替换默认 CTest 的内存运行时 smoke。