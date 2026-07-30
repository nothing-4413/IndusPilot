# Change: 强化本地依赖 compose 安全基线

## Why

当前 `deployment/docker-compose.yml` 直接提交固定数据库密码，并默认暴露 MySQL、Redis、MongoDB 到所有网络接口。它适合快速演示，但容易被误用到生产或联调环境，且缺少 healthcheck 会降低依赖启动状态的可观测性。

## What Changes

- 将 MySQL、Redis、MongoDB 密钥改为从 `deployment/.env` 显式读取，并提交 `.env.example` 作为模板。
- 默认端口绑定改为 `127.0.0.1`，需要远程访问时通过 `.env` 显式放开。
- 为 MySQL、Redis、MongoDB 增加容器 healthcheck。
- 部署预检新增密钥、回环绑定和 healthcheck 检查。
- 文档更新为先复制 `.env.example` 再启动 compose。

## Non-Goals

- 不引入 Docker Swarm、Kubernetes 或密钥管理服务。
- 不改变后端默认内存仓储演示模式。
- 不强制本机必须安装 Docker 才能通过离线预检。