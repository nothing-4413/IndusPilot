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
- `GET /api/v1/assets`：需要 `asset:read` 权限，返回资产列表；支持 `factory`、`workshop`、`productionLine`、`status` 查询参数。
- `GET /api/v1/assets/{id}`：需要 `asset:read` 权限，返回单个资产；不存在时返回 `RESOURCE_NOT_FOUND`。
- `POST /api/v1/assets`：需要 `asset:write` 权限，创建或更新资产。`status` 可取 `active`、`inactive`、`maintenance`、`retired`。
- `PATCH /api/v1/assets/{id}/status`：需要 `asset:write` 权限，仅更新资产生命周期状态，不改变资产层级和身份字段。
- `GET /api/v1/monitoring/states`：需要 `asset:read` 权限，返回运行状态列表、状态汇总和严重度汇总。
- `GET /api/v1/monitoring/states/{assetId}`：需要 `asset:read` 权限，返回单个资产的当前运行状态。
- `POST /api/v1/monitoring/states`：需要 `monitoring:write` 权限，写入短生命周期运行状态；`state` 可取 `online`、`warning`、`critical`、`offline`，`severity` 可取 `info`、`warning`、`critical`。
- `GET /api/v1/alerts`：需要 `alert:read` 权限，返回告警列表；支持 `assetId`、`severity`、`state` 查询参数。
- `GET /api/v1/alerts/{id}`：需要 `alert:read` 权限，返回单个告警。
- `POST /api/v1/alerts`：需要 `alert:write` 权限，创建告警；`severity` 可取 `info`、`warning`、`critical`。
- `POST /api/v1/alerts/{id}/acknowledge`：需要 `alert:write` 权限，确认告警。
- `POST /api/v1/alerts/{id}/assign`：需要 `alert:write` 权限，分派告警，字段为 `assignee`。
- `POST /api/v1/alerts/{id}/resolve`：需要 `alert:write` 权限，解决告警。
- `POST /api/v1/alerts/{id}/close`：需要 `alert:write` 权限，关闭告警。
- `GET /api/v1/work-orders`：需要 `work-order:read` 权限，返回工单列表；支持 `assetId`、`alertId`、`state` 查询参数。
- `GET /api/v1/work-orders/{id}`：需要 `work-order:read` 权限，返回单个工单。
- `POST /api/v1/work-orders`：需要 `work-order:write` 权限，创建工单。
- `POST /api/v1/work-orders/from-alert`：需要 `work-order:write` 权限，从告警创建工单。
- `POST /api/v1/work-orders/{id}/assign`：需要 `work-order:write` 权限，分派工单，字段为 `assignee`。
- `POST /api/v1/work-orders/{id}/start`：需要 `work-order:write` 权限，开始处理工单。
- `POST /api/v1/work-orders/{id}/complete`：需要 `work-order:write` 权限，完成工单，字段为 `result`。
- `POST /api/v1/work-orders/{id}/close`：需要 `work-order:write` 权限，关闭工单。
- `GET /api/v1/assets/{assetId}/maintenance-history`：需要 `work-order:read` 权限，返回资产已关闭维护历史。
- `GET /api/v1/ai/status`：需要 `ai:use` 权限，返回 AI 模块状态。
- `POST /api/v1/ai/troubleshoot`：需要 `ai:use` 权限，提交 `relatedType`、`relatedId`、`prompt` 和可选 `contextItems`，返回非权威故障排查建议或不可用说明。
- `POST /api/v1/ai/summarize-logs`：需要 `ai:use` 权限，提交日志或上下文摘要请求。
- `GET /api/v1/ai/interactions`：需要 `ai:use` 权限，查询 AI 交互审计记录；支持 `relatedType` 和 `relatedId` 查询参数。

接口响应统一使用：`success`、`code`、`message`、`data`。

## 后续约束

当前 HTTP 层已经接入会话守卫、权限守卫、统一错误响应和资产仓储边界。资产服务默认使用内存仓储，运行监控状态默认保存在进程内存中，便于离线演示和集成测试；后续可以增加配置项切换到 MySQL/Redis/MongoDB 等真实存储，并继续接入告警、工单和 AI 诊断的写接口。

## 错误响应

- INVALID_REQUEST：请求体缺少必要字段或格式无效。
- AUTHENTICATION_REQUIRED：缺少有效会话或会话已过期。
- AUTHORIZATION_DENIED：用户缺少当前接口所需权限。
- RESOURCE_NOT_FOUND：目标资源不存在。

## 集成测试

`dev-http` preset 会在 Windows 下注册 `induspilot-http-integration-smoke` CTest。该测试会启动本地后端，覆盖健康检查、登录、受保护路由、权限拒绝、请求校验、资源不存在错误、资产层级筛选、资产生命周期状态变更、运行状态写入、运行状态详情、监控汇总、告警创建、告警筛选、告警生命周期流转、工单创建、工单生命周期流转、资产维修历史、AI 辅助请求和 AI 交互审计。

手动运行：

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File backend/tests/http_integration_smoke.ps1 `
  -BackendExe build/dev-http/backend/induspilot-backend.exe `
  -ConfigPath config/backend.example.yaml
```

如果已经启动 Redis，可额外传入 `-SessionStore redis` 验证 Redis-backed session。