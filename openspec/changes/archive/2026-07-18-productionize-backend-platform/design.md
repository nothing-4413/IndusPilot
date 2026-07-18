# Design: productionize-backend-platform

## 目标

本阶段先把后端从进程内模块演示推进到真实 HTTP 服务入口，同时保持现有业务模块、测试和 Qt 客户端构建不被破坏。HTTP 层只做适配，不把 Drogon 类型泄漏到核心业务模块。

## 框架选择

中心后端 HTTP 服务选择 Drogon。它满足当前阶段需要：

- C++17 项目可直接集成。
- 支持异步 HTTP、JSON 响应和路由注册。
- vcpkg 提供 MySQL、Redis、ORM、YAML 特性，便于后续继续推进数据仓储和配置化依赖。
- Windows 本地可验证，Linux 部署路径也清晰。

Boost.Beast 保留给后续边缘网关或工业协议适配器，Oat++ 保留为 OpenAPI/DTO 服务风格备选。

## 代码结构

新增 HTTP 适配层：

- `backend/include/induspilot/http/drogon_server.hpp`
- `backend/src/http/drogon_server.cpp`

`main.cpp` 在 `INDUSPILOT_WITH_DROGON=ON` 时启动 Drogon 服务，否则保留原来的单进程健康检查输出。这样默认构建、Qt 客户端构建和基础测试不会被 HTTP 框架强绑定。

## 构建策略

新增 CMake 选项：

- `INDUSPILOT_WITH_DROGON`：启用 Drogon HTTP 服务运行时。

新增 preset：

- `dev-http`：启用 vcpkg toolchain、Redis client 和 Drogon，构建后端 HTTP 服务和测试。

## 首批接口

首批接口覆盖服务启动和模块访问骨架：

- `GET /health`
- `POST /api/v1/auth/login`
- `GET /api/v1/auth/session`
- `POST /api/v1/auth/logout`
- `GET /api/v1/assets`
- `GET /api/v1/monitoring/states`
- `GET /api/v1/alerts`
- `GET /api/v1/work-orders`
- `GET /api/v1/ai/status`

响应统一使用 `success`、`code`、`message`、`data` 包装。

## 后续工作

后续任务继续推进结构化配置、Redis session 配置化、MySQL 仓储、认证中间件、权限守卫、请求校验、结构化日志和接口级集成测试。