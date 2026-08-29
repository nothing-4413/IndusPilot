# 生产就绪与整体协调检查

本文档用于说明 IndusPilot 当前已经具备的生产化基础、仍需控制的边界，以及下一阶段优先级。它不是上线声明，而是项目演进时的协调基线。

## 已具备

- 模块边界：身份权限、资产、运行监控、告警、工单和 AI 辅助诊断已经拆分为独立服务与仓储接口。
- HTTP 运行时：Drogon 后端提供统一 JSON 响应、认证守卫、权限守卫、业务路由和可参数化 HTTP 冒烟测试；默认内存模式用于 CTest，真实运行时 profile runner 可读取 `deployment/.env` 并切换 MySQL 仓储和 Redis session；`/health/live`、`/health/ready`、`/health/startup` 已分离进程存活、依赖准入和初始化状态，并支持 SIGTERM/SIGINT、draining gate、在途请求计数和 listener 失败退出码。
- 持久化边界：`storage.repository_store=memory/mysql` 可以切换内存仓储和 MySQL 仓储；MySQL 已覆盖身份、资产、告警、工单、运行状态和 AI 交互审计。
- 会话边界：默认内存会话适合本地开发，Redis-backed session 可通过 `redis.session_store=redis` 或环境变量启用。
- AI 边界：AI 模块保持非阻塞，支持 disabled/http provider 配置、agent 诊断编排、降级结果和交互审计。
- 工程流程：OpenSpec 变更、任务清单、CMake preset、数据库脚本、schema 版本登记、部署 compose、部署前预检、HTTP 冒烟测试、真实依赖 CRUD smoke、密钥扫描和 GitHub Actions CI 已经进入仓库。

## 当前边界

- 身份安全：当前已支持版本化 PBKDF2-SHA256 密码校验、登录失败锁定、可配置密码策略、认证后密码轮换、密码轮换审计、按用户撤销全部 session 和密钥扫描门禁，并保留显式开发兼容格式；生产前仍必须替换演示口令、执行首登/种子账号治理、验证分布式限流、最小权限账户和审计保留策略。
- 依赖健康：`/health` 保持旧客户端的 200 兼容语义；`/health/ready` 按 `repository_store`、`session_store` 和 AI required 配置判断核心依赖，使用 single-flight、并发依赖探测、统一 DNS/connect deadline 和缓存重新评估恢复状态，并暴露探测次数、耗时、失败/恢复计数；部署前预检会检查离线 schema 版本基线，CI dependency smoke 会验证真实 MySQL/Redis/MongoDB 启动、认证、迁移幂等、MySQL 核心业务 CRUD、Redis 数据结构读写和 MongoDB 文档 CRUD。
- 配置边界：配置文件读取失败、未知字段、错误层级和非法整数/布尔值会在 listener 启动前阻止进程启动，并以退出码 `78` 报告；外部依赖暂时不可用仍由 readiness `503` 表达。
- 停机边界：收到 SIGTERM/SIGINT 后新业务请求返回 `503/SERVER_DRAINING`，readiness 立即为 `503`、liveness 在最终停止前保持 `200`，coordinator 在 `shutdown.drain_timeout_ms` deadline 内等待已接受请求完成，超时记录剩余请求并退出；listener 绑定预检失败返回退出码 `69`。
- AI 传输：Drogon 构建下的 `provider=http` 已通过配置 endpoint 发起受控 JSON POST，并支持鉴权头、超时、响应文本提取和失败降级；当前结构化诊断字段仍由本地编排器生成。`/metrics` 额外暴露 provider 调用、可用/降级结果和耗时指标，标签不包含 prompt、响应、凭据或业务编号。`backend/tests/ai_provider_http_smoke.ps1` 覆盖成功、非 2xx、非 JSON 和超时场景。
- 通知通道：`console` 具备本地投递；`email` 明确保持未实现；`webhook` 为显式开关控制且受请求超时约束，失败不会伪造 sent 状态，而是进入通知队列的重试/死信流程。
- Webhook 出站安全：启用 webhook 必须提供精确 host allowlist；目标 host 不匹配时在建立 HTTP 连接前拒绝，避免规则配置直接形成任意出站探测。
- MongoDB：当前尚未接入后端业务仓储；CI 已验证初始化集合、索引和文档 CRUD，非结构化日志、知识片段和长上下文仍待正式落库。
- 客户端：Qt 客户端已接入 HTTP 登录、资产列表与状态更新、运行监控列表与状态写入、告警创建/规则/通知投递/列表与处置、维护工单列表、新建/编辑/附件/从告警生成/分派/基础流转、AI 结构化诊断入口和 AI 交互审计查询、分页与 CSV 导出，并接入告警规则/通知联动。
- 集成测试：默认 HTTP 冒烟测试覆盖内存仓储；CI dependency smoke 已覆盖 MySQL、Redis、MongoDB 的真实依赖启动、MySQL 核心业务 CRUD、Redis 数据结构读写和 MongoDB 文档 CRUD。

## 下一阶段优先级

1. 真实依赖集成深化：CI 已在独立的 `backend-runtime-profile` job 中构建 HTTP runtime、启动 MySQL/Redis/MongoDB、执行依赖 CRUD smoke，并运行 MySQL 仓储 + Redis session HTTP profile；下一步推进 MongoDB 业务仓储接入。
2. 身份安全深化：在现有密码哈希、登录失败锁定、审计、密码轮换和按用户 session 撤销边界上，补充种子账号替换/首登治理和跨副本登录失败限流。
3. Qt 客户端联机化深化：在现有 HTTP 登录、资产、运行监控列表与状态写入、告警创建/规则/通知投递/列表与处置、维护工单列表、新建/编辑/附件/从告警生成/分派/基础流转、AI 诊断入口和 AI 交互审计查询、分页与 CSV 导出基础上，继续接入真实外部通知通道适配器、异步重试队列和投递指标。
4. 外部 AI Provider：在已有 HTTP 传输、有限重试、总超时、响应大小限制、常见凭据键值脱敏和降级审计基础上，继续实现提示词版本、结构化响应协议和 provider 运行指标。
5. 可观测性：在已有结构化请求日志、请求追踪和关键业务指标基础上，补充 readiness 状态变化指标、日志脱敏/保留策略和运行告警。
6. CI/CD 扩展：基础 CI、质量门禁、安全扫描、配置预检、OpenSpec 校验、真实依赖 smoke、Drogon/vcpkg HTTP 构建矩阵和 14 天 CI 构建证据 artifact 已接入；后续继续增加正式制品发布流水线和部署环境验证。

## 验证命令

```powershell
cmake --preset dev-http
cmake --build --preset dev-http
ctest --preset dev-http
openspec validate --specs --strict
openspec validate production-readiness-cleanup --strict
openspec validate persist-runtime-modules --strict
openspec validate agent-diagnosis-orchestration --strict
```

## 协调原则

- 默认路径优先服务开发演示，生产能力通过显式配置启用。
- 优雅停机必须设置适合业务最长请求耗时的 `shutdown.drain_timeout_ms`，并监控 timeout 日志中的剩余请求数量。
- 所有 AI 建议必须可降级、可审计，并保留人工复核。
- 新模块先进入 OpenSpec，再实现仓储、服务、HTTP、测试和文档，最后按模块提交。

## 身份凭据轮换边界

密码轮换已经具备配置、仓储、服务、HTTP 和 smoke 覆盖，并在成功后撤销该用户全部 session（包括当前 session）。它不会自动替换种子账号；部署编排必须显式执行首轮口令治理，并把分布式限流作为后续变更管理。Redis session store 会维护用户索引，并扫描既有 session key 兼容升级前会话。

## 操作审计生产边界

操作审计已经具备统一事件模型、权限保护查询、内存/MySQL 仓储和 Qt 查询页面，适合展示关键操作可追踪能力。生产落地前仍建议继续扩展：服务端分页和筛选、不可变审计策略、日志归档保留周期、敏感字段脱敏、外部 SIEM 对接、审计事件失败重试和多租户隔离。

## 操作审计查询能力

操作审计已支持按操作者、动作、资源类型、结果和包含边界的发生时间范围筛选，并支持兼容式分页响应。生产增强仍建议加入时间范围数据库索引、服务端排序、模糊检索、审计数据归档策略和不可变存储约束。

## 操作审计导出边界

操作审计导出已使用独立 `audit:export` 权限保护，并会记录导出行为自身。生产环境继续建议补充导出审批、文件水印、敏感字段脱敏、下载频率限制、异步大文件导出和导出文件保留策略。
