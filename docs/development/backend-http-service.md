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
- `POST /api/v1/ai/diagnose`：需要 `ai:use` 权限，提交 `relatedType`、`relatedId`、`prompt` 和 `context`，返回结构化诊断结果。
- `GET /api/v1/ai/interactions`：需要 `ai:use` 权限，查询 AI 交互审计记录；支持 `relatedType`、`relatedId`、`limit` 和 `offset` 查询参数；未传分页参数时返回数组，传入分页参数时返回 `{ items, total, limit, offset }`。

接口响应统一使用：`success`、`code`、`message`、`data`。

## 后续约束

当前 HTTP 层已经接入会话守卫、权限守卫、统一错误响应和仓储边界。`storage.repository_store` 为 `memory` 时使用内存仓储；设置为 `mysql` 时，身份认证、资产、告警、工单、运行状态和 AI 交互审计使用 MySQL 仓储。AI 模块会读取 `ai.enabled`、`ai.provider` 与 `ai.endpoint`，通过 Provider 边界生成结构化 agent 诊断结果并写入审计；当前 `disabled/http` provider 都使用本地规则降级，尚未执行外部推理传输。

## Qt 客户端联机

Qt 客户端会读取 `config/client.example.json` 中的 `apiBaseUrl`。当前已接入：

- `POST /api/v1/auth/login`：登录成功后保存 Bearer token。
- `GET /api/v1/assets`：登录后同步资产列表。
- `PATCH /api/v1/assets/{id}/status`：选中资产后更新生命周期状态。
- `GET /api/v1/monitoring/states`：登录后同步运行监控列表。
- `POST /api/v1/monitoring/states`：登录后写入设备运行状态、严重度和指标摘要，成功后刷新列表。
- `GET /api/v1/alerts`：登录后同步告警列表。
- `POST /api/v1/alerts`：创建现场告警，提交告警编号、设备编号、级别、标题、状态和负责人。
- `POST /api/v1/alerts/{id}/acknowledge`：确认选中告警。
- `POST /api/v1/alerts/{id}/assign`：分派选中告警。
- `POST /api/v1/alerts/{id}/resolve`：解决选中告警。
- `POST /api/v1/alerts/{id}/close`：关闭选中告警。
- `GET /api/v1/work-orders`：登录后同步维护工单列表。
- `POST /api/v1/work-orders`：创建维护工单。
- `POST /api/v1/work-orders/from-alert`：基于选中告警填写处置摘要并生成关联工单。
- `POST /api/v1/work-orders/{id}/assign`：选中工单后分派处理人。
- `POST /api/v1/work-orders/{id}/start`：选中工单后开始处理。
- `POST /api/v1/work-orders/{id}/complete`：填写处理结果并完成工单。
- `POST /api/v1/work-orders/{id}/close`：关闭已完成工单。
- `POST /api/v1/ai/diagnose`：提交结构化上下文并展示 AI 诊断结果。
- `GET /api/v1/ai/interactions`：按当前关联类型和对象分页同步 AI 交互审计，客户端可将当前表格导出为 CSV。

如果后端不可达，客户端会保留离线演示数据并在登录页提示当前模式。AI Provider 不可用时诊断页展示降级建议，交互审计查询失败时展示离线记录；工单编辑/附件和告警规则/通知联动后续按模块接入。

## 错误响应

- INVALID_REQUEST：请求体缺少必要字段或格式无效。
- AUTHENTICATION_REQUIRED：缺少有效会话或会话已过期。
- AUTHORIZATION_DENIED：用户缺少当前接口所需权限。
- RESOURCE_NOT_FOUND：目标资源不存在。

## 集成测试

`dev-http` preset 会在 Windows 下注册 `induspilot-http-integration-smoke` CTest。该测试会启动本地后端，覆盖健康检查、登录、受保护路由、权限拒绝、请求校验、资源不存在错误、资产层级筛选、资产生命周期状态变更、运行状态写入、运行状态详情、监控汇总、告警创建、告警筛选、告警生命周期流转、工单创建、工单生命周期流转、资产维修历史、AI 辅助请求、AI 交互审计和分页参数校验。

手动运行：

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File backend/tests/http_integration_smoke.ps1 `
  -BackendExe build/dev-http/backend/induspilot-backend.exe `
  -ConfigPath config/backend.example.yaml
```

如果已经启动 Redis，可额外传入 `-SessionStore redis` 验证 Redis-backed session。
