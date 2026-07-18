# 后端 HTTP 服务

## 构建

Drogon 已通过 vcpkg 安装，启用中心后端 HTTP 服务时使用：

```powershell
cmake --preset dev-http
cmake --build --preset dev-http
ctest --preset dev-http
```

## 启动

```powershell
.\build\dev-http\backend\induspilot-backend.exe config\backend.example.yaml
```

默认监听配置来自 `config/backend.example.yaml`，当前示例为 `0.0.0.0:8080`。

## 首批接口

- `GET /health`：返回服务健康状态和依赖占位探测结果。
- `POST /api/v1/auth/login`：使用 JSON 请求体登录，字段为 `username` 和 `password`。
- `GET /api/v1/auth/session`：使用 `Authorization: Bearer <token>` 验证会话。
- `POST /api/v1/auth/logout`：使用 `Authorization: Bearer <token>` 退出会话。
- `GET /api/v1/assets`：需要 `asset:read` 权限，返回资产列表。
- `POST /api/v1/assets`：需要 `asset:write` 权限，创建或更新资产。
- `GET /api/v1/monitoring/states`：需要 `asset:read` 权限，返回运行状态汇总。
- `GET /api/v1/alerts`：需要 `alert:read` 权限，返回告警列表。
- `GET /api/v1/work-orders`：需要 `work-order:read` 权限，返回工单列表。
- `GET /api/v1/ai/status`：需要 `ai:use` 权限，返回 AI 模块状态。

接口响应统一使用：`success`、`code`、`message`、`data`。

## 后续约束

当前 HTTP 层是生产化入口的第一步，仍保留内存业务服务用于演示和测试。后续需要继续接入配置化 Redis session、MySQL 仓储、认证中间件、权限守卫和请求校验。