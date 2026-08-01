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

- `GET /health`：返回服务健康状态和 MySQL、Redis、MongoDB、AI 依赖连通性探测结果。
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
- `GET /api/v1/alert-rules`：需要 `alert:read` 权限，返回告警规则列表。
- `POST /api/v1/alert-rules`：需要 `alert:write` 权限，创建告警规则；`minSeverity` 可取 `info`、`warning`、`critical`。
- `GET /api/v1/alert-notifications`：需要 `alert:read` 权限，返回告警命中规则后生成的通知记录。
- `POST /api/v1/alert-notifications/dispatch`：需要 `alert:write` 权限，投递 `queued/retrying` 通知并返回成功、失败和跳过统计。
- `POST /api/v1/alert-notifications/{id}/retry`：需要 `alert:write` 权限，重试单条未成功通知。
- `POST /api/v1/alerts/{id}/acknowledge`：需要 `alert:write` 权限，确认告警。
- `POST /api/v1/alerts/{id}/assign`：需要 `alert:write` 权限，分派告警，字段为 `assignee`。
- `POST /api/v1/alerts/{id}/resolve`：需要 `alert:write` 权限，解决告警。
- `POST /api/v1/alerts/{id}/close`：需要 `alert:write` 权限，关闭告警。
- `GET /api/v1/work-orders`：需要 `work-order:read` 权限，返回工单列表；支持 `assetId`、`alertId`、`state` 查询参数。
- `GET /api/v1/work-orders/{id}`：需要 `work-order:read` 权限，返回单个工单。
- `POST /api/v1/work-orders`：需要 `work-order:write` 权限，创建工单。
- `POST /api/v1/work-orders/from-alert`：需要 `work-order:write` 权限，从告警创建工单。
- `PATCH /api/v1/work-orders/{id}`：需要 `work-order:write` 权限，编辑工单摘要、处理人和处理结果。
- `GET /api/v1/work-orders/{id}/attachments`：需要 `work-order:read` 权限，查询工单附件元数据。
- `POST /api/v1/work-orders/{id}/attachments`：需要 `work-order:write` 权限，登记工单附件元数据。
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
- `GET /api/v1/audit/events`：需要 `audit:read` 权限，查询操作审计事件；支持 `actor`、`action`、`resourceType`、`result`、`limit` 和 `offset` 查询参数。
- `GET /api/v1/audit/events/export`：需要 `audit:export` 权限，按当前筛选条件导出操作审计 CSV。
- `GET /api/v1/audit/integrity`：需要 `audit:read` 权限，复算操作审计哈希链并返回完整性状态。

接口响应统一使用：`success`、`code`、`message`、`data`。

## 后续约束

当前 HTTP 层已经接入会话守卫、权限守卫、统一错误响应和仓储边界。`storage.repository_store` 为 `memory` 时使用内存仓储；设置为 `mysql` 时，身份认证、资产、告警、告警规则、告警通知投递审计、工单、运行状态和 AI 交互审计使用 MySQL 仓储。AI 模块会读取 `ai.enabled`、`ai.provider` 与 `ai.endpoint`，通过 Provider 边界生成结构化 agent 诊断结果并写入审计；当前 `disabled/http` provider 都使用本地规则降级，尚未执行外部推理传输。

## Qt 客户端联机

Qt 客户端会读取 `config/client.example.json` 中的 `apiBaseUrl`。当前已接入：

- `POST /api/v1/auth/login`：登录成功后保存 Bearer token。
- `GET /api/v1/assets`：登录后同步资产列表。
- `PATCH /api/v1/assets/{id}/status`：选中资产后更新生命周期状态。
- `GET /api/v1/monitoring/states`：登录后同步运行监控列表。
- `POST /api/v1/monitoring/states`：登录后写入设备运行状态、严重度和指标摘要，成功后刷新列表。
- `GET /api/v1/alerts`：登录后同步告警列表。
- `POST /api/v1/alerts`：创建现场告警，提交告警编号、设备编号、级别、标题、状态和负责人。
- `GET /api/v1/alert-rules` / `POST /api/v1/alert-rules`：在告警中心查看并新增规则。
- `GET /api/v1/alert-notifications`：在告警中心查看通知记录。
- `POST /api/v1/alert-notifications/dispatch` / `POST /api/v1/alert-notifications/{id}/retry`：在告警通知对话框投递队列或重试选中记录。
- `POST /api/v1/alerts/{id}/acknowledge`：确认选中告警。
- `POST /api/v1/alerts/{id}/assign`：分派选中告警。
- `POST /api/v1/alerts/{id}/resolve`：解决选中告警。
- `POST /api/v1/alerts/{id}/close`：关闭选中告警。
- `GET /api/v1/work-orders`：登录后同步维护工单列表。
- `POST /api/v1/work-orders`：创建维护工单。
- `POST /api/v1/work-orders/from-alert`：基于选中告警填写处置摘要并生成关联工单。
- `PATCH /api/v1/work-orders/{id}`：编辑选中工单的摘要、处理人和处理结果。
- `GET /api/v1/work-orders/{id}/attachments`：查看选中工单附件元数据。
- `POST /api/v1/work-orders/{id}/attachments`：登记选中工单附件元数据。
- `POST /api/v1/work-orders/{id}/assign`：选中工单后分派处理人。
- `POST /api/v1/work-orders/{id}/start`：选中工单后开始处理。
- `POST /api/v1/work-orders/{id}/complete`：填写处理结果并完成工单。
- `POST /api/v1/work-orders/{id}/close`：关闭已完成工单。
- `POST /api/v1/ai/diagnose`：提交结构化上下文并展示 AI 诊断结果。
- `GET /api/v1/ai/interactions`：按当前关联类型和对象分页同步 AI 交互审计，客户端可将当前表格导出为 CSV。
- `GET /api/v1/audit/events`：分页同步操作审计记录，展示哈希链字段。
- `GET /api/v1/audit/events/export`：按当前筛选条件下载操作审计 CSV。
- `GET /api/v1/audit/integrity`：刷新操作审计完整性状态。

如果后端不可达，客户端会保留离线演示数据并在登录页提示当前模式。AI Provider 不可用时诊断页展示降级建议，交互审计查询失败时展示离线记录；告警规则/通知联动和通知投递状态流转已接入，外部邮件、Webhook、短信等真实通道适配器作为后续模块扩展。

## 错误响应

- INVALID_REQUEST：请求体缺少必要字段或格式无效。
- AUTHENTICATION_REQUIRED：缺少有效会话或会话已过期。
- AUTHORIZATION_DENIED：用户缺少当前接口所需权限。
- RESOURCE_NOT_FOUND：目标资源不存在。

## 集成测试

`dev-http` preset 会在 Windows 下注册 `induspilot-http-integration-smoke` CTest。该测试会启动本地后端，覆盖健康检查、登录、受保护路由、权限拒绝、请求校验、资源不存在错误、资产层级筛选、资产生命周期状态变更、运行状态写入、运行状态详情、监控汇总、告警创建、告警筛选、告警生命周期流转、告警规则创建、告警通知生成、告警通知投递、工单创建、工单生命周期流转、资产维修历史、AI 辅助请求、AI 交互审计、操作审计筛选分页、CSV 导出、哈希链完整性校验和分页参数校验。

手动运行：

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File backend/tests/http_integration_smoke.ps1 `
  -BackendExe build/dev-http/backend/induspilot-backend.exe `
  -ConfigPath config/backend.example.yaml
```

如果已经启动 Redis，可额外传入 `-SessionStore redis` 验证 Redis-backed session。

## 操作审计接口

- `GET /api/v1/audit/events`：需要 `audit:read` 权限，返回最近操作审计事件数组；传入分页参数时返回 `{ items, total, limit, offset }`。
- `GET /api/v1/audit/integrity`：需要 `audit:read` 权限，返回 `verified`、`total`、`brokenEventId` 和 `latestHash`，用于校验审计哈希链是否被篡改。
- 登录成功会写入 `auth.login` 审计事件。
- `POST /api/v1/alert-notifications/dispatch` 会写入 `alert-notification.dispatch` 审计事件，资源编号包含 `sent/failed/skipped` 摘要。
- `POST /api/v1/alert-notifications/{id}/retry` 会写入 `alert-notification.retry` 审计事件。

默认内存权限仅为 `admin` 授予 `audit:read` 与 `audit:export`；MySQL 种子脚本会把 `audit:read`、`audit:export` 加入权限表，并由管理员角色自动继承。

## 操作审计筛选与分页

`GET /api/v1/audit/events` 支持 `actor`、`action`、`resourceType`、`result` 查询参数。为了兼容既有调用，未传 `limit` 和 `offset` 时 `data` 仍为数组；传入分页参数时 `data` 为 `{ items, total, limit, offset }`。`limit` 合法范围为 1 到 100，`offset` 合法范围为 0 到 1000000。

## 操作审计 CSV 导出

`GET /api/v1/audit/events/export` 需要 `audit:export` 权限，返回 `text/csv; charset=utf-8` 内容，并支持与审计查询一致的 `actor`、`action`、`resourceType`、`result` 筛选参数。默认管理员拥有该权限，operator 和 maintainer 不具备。导出成功后系统会写入 `operation-audit.export` 审计事件，资源编号包含导出数量摘要。

## 操作审计完整性校验

新写入的操作审计事件会自动生成 `previousHash` 和 `eventHash`。首条事件的 `previousHash` 为 `genesis`，后续事件的 `previousHash` 指向上一条事件的 `eventHash`。`GET /api/v1/audit/integrity` 会按写入顺序复算哈希链，若发现内容或链路字段被篡改，返回 `verified=false` 和首个断点事件编号。
## 请求追踪

HTTP 服务会读取 `X-Trace-Id` 或 `X-Request-Id`，优先使用 `X-Trace-Id`，未提供时生成 `trace-<timestamp>-<sequence>`。所有响应都会回传 `X-Trace-Id` 与 `X-Request-Id`，结构化请求日志中的 `traceId` 与操作审计 `traceId` 使用同一值，便于从 API 调用追踪到审计记录。
## 登录失败锁定

HTTP 登录接口复用身份服务的安全策略。`security.login_lockout_enabled` 开启后，同一用户名在 `security.login_failure_window_seconds` 窗口内连续失败达到 `security.login_max_failures`，后续登录会返回 `429 Too Many Requests`，并通过 `Retry-After` 告知剩余锁定时间。失败和锁定事件分别写入 `auth.login.failed`、`auth.login.locked` 操作审计，审计事件沿用当前请求追踪编号。
## AI Provider 鉴权与响应校验

HTTP AI Provider 支持通过 `ai.api_key`、`ai.auth_header`、`ai.auth_scheme` 配置鉴权头，生产环境建议使用 `INDUSPILOT_AI_API_KEY` 注入密钥而不是写入配置文件。默认开启 `ai.require_structured_response`，Provider 响应必须是 JSON，并提供 `content`、`summary`、`text`、`output_text` 或兼容 OpenAI `choices/output` 的文本字段；缺少可用文本时系统会降级为不可用结果并继续记录 AI 交互。
## 可观测性指标

启用 Drogon 运行时时，后端提供 `GET /metrics`，返回 Prometheus 文本格式指标。指标覆盖 HTTP 请求总数、错误数、AI 请求数、告警关闭次数、工单关闭次数以及按方法、归一化路径和状态码聚合的请求耗时。详细说明见 `docs/development/observability-metrics.md`。
