# Change: 参数化 HTTP 真实运行时 smoke

## Why

现有 HTTP smoke 已经覆盖核心业务接口，但默认固定使用内存仓储，只能证明服务层和路由在本地模式下可用。随着 MySQL、Redis、MongoDB dependency smoke 已进入 CI，需要让同一套 HTTP smoke 能按参数切换到真实仓储和 Redis session，为后续真实依赖端到端 CI 作业提供稳定入口。

## What Changes

- 为 `backend/tests/http_integration_smoke.ps1` 增加 `-RepositoryStore` 参数，支持 `memory/mysql`。
- 为 HTTP smoke 增加 MySQL、Redis、MongoDB 连接覆盖参数，便于在 compose 依赖环境中复用同一脚本。
- 保持默认 CTest 使用 `memory` 仓储和 `memory` session，不增加本地开发依赖。
- 更新文档，说明内存模式和真实依赖模式的运行命令与边界。

## Non-Goals

- 本次不新增 GitHub Actions 中的 Drogon/vcpkg 真实仓储构建矩阵。
- 本次不修改后端业务仓储实现。