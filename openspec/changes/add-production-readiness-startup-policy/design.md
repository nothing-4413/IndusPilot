# 设计：生产 readiness 与启动策略

## 状态模型

```text
配置无效 ───────────────▶ startup failed，进程退出
配置有效 + 依赖未就绪 ───▶ startup initialized，liveness 200，readiness 503
配置有效 + 必需依赖就绪 ─▶ startup initialized，liveness 200，readiness 200
依赖恢复 ───────────────▶ 下次 readiness 检查重新探测并返回 200
```

`Application::start()` 只负责静态配置校验和初始依赖状态初始化；依赖不可用不是启动失败。静态错误不启动 HTTP listener，调用方通过返回值决定退出码。

## 依赖需求

| 依赖 | required 条件 | 未就绪影响 |
|---|---|---|
| MySQL | `storage.repository_store=mysql` | readiness 503 |
| Redis | `redis.session_store=redis` | readiness 503 |
| MongoDB | 当前无业务仓储 | 不阻断，不探测为核心依赖 |
| AI | `ai.enabled=true` 且 provider 为 `http`；只有 `ai.required=true` 才 required | 默认降级；显式 required 时 readiness 503 |

memory 模式不探测未使用的 MySQL、Redis；可选依赖不作为 readiness 失败条件。响应中同时暴露每个依赖的 `required`、`available` 和 `reason`，避免用单一布尔值混淆“未配置”和“不可用”。

## HTTP 契约

- `GET /health`：保持现有 service/dependencies/warnings JSON 和 HTTP 200 兼容语义。
- `GET /health/live`：进程已初始化并提供 HTTP 服务时返回 200；不执行外部依赖探测。
- `GET /health/ready`：按缓存策略刷新依赖状态；全部 required 依赖可用返回 200，否则返回 503。
- `GET /health/startup`：返回配置校验、初始化状态和失败原因；初始化完成返回 200，未完成或失败返回 503。

所有新端点继续使用现有 trace header、结构化请求日志和 JSON response helper。

## 探测和缓存

依赖探测使用配置的连接端点，并设置单次超时。Application 保存最近一次状态和探测时间，在 `probe_cache_ms` 内复用结果；缓存过期后 readiness 请求重新探测，允许依赖恢复生效。健康检查不执行认证、schema migration 或业务写入，MySQL/Redis 的更强 CRUD 验证继续由真实依赖 smoke 负责。

## 边界

配置解析、静态校验和依赖需求模型属于 app/data foundation；HTTP 状态码和 JSON 映射属于 platform route registrar。业务 route registrar 不读取全局配置，也不自行探测依赖。
