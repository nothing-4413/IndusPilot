# Change: Qt 客户端 HTTP 联机入口

## Why

Qt 客户端当前明确是离线演示，所有数据来自本地静态数据。为了让项目展示端到端 agent 能力，需要先接入真实 HTTP 登录与基础资产列表，同时保留离线兜底，避免后端未启动时客户端不可用。

## What Changes

- 客户端读取 `config/client.example.json` 中的 `apiBaseUrl`。
- 登录时调用 `POST /api/v1/auth/login`，保存返回的 Bearer token 和当前用户。
- 登录成功后调用 `GET /api/v1/assets`，使用真实资产数据刷新资产列表。
- HTTP 失败或后端不可用时保留离线演示数据，并在界面中提示当前运行模式。

## Non-Goals

- 不在本次接入运行监控、告警、工单和 AI 页面 HTTP 数据。
- 不实现 token 持久化、刷新、自动重登或安全存储。
- 不重构整体 UI 设计和导航结构。