# 生产就绪与整体协调检查

本文档用于说明 IndusPilot 当前已经具备的生产化基础、仍需控制的边界，以及下一阶段优先级。它不是上线声明，而是项目演进时的协调基线。

## 已具备

- 模块边界：身份权限、资产、运行监控、告警、工单和 AI 辅助诊断已经拆分为独立服务与仓储接口。
- HTTP 运行时：Drogon 后端提供统一 JSON 响应、认证守卫、权限守卫、业务路由和 HTTP 冒烟测试。
- 持久化边界：`storage.repository_store=memory/mysql` 可以切换内存仓储和 MySQL 仓储；MySQL 已覆盖身份、资产、告警、工单、运行状态和 AI 交互审计。
- 会话边界：默认内存会话适合本地开发，Redis-backed session 可通过 `redis.session_store=redis` 或环境变量启用。
- AI 边界：AI 模块保持非阻塞，支持 disabled/http provider 配置、agent 诊断编排、降级结果和交互审计。
- 工程流程：OpenSpec 变更、任务清单、CMake preset、数据库脚本、schema 版本登记、部署 compose、部署前预检、集成测试和 GitHub Actions 基础 CI 已经进入仓库。

## 当前边界

- 身份安全：当前已支持版本化 PBKDF2-SHA256 密码校验，并保留显式开发兼容格式；生产前仍必须替换演示盐值、补齐密码轮换、登录失败锁定、审计和最小权限账户治理。
- 依赖健康：`/health` 当前只做 TCP 连通性探测；部署前预检会检查离线 schema 版本基线，但仍不替代真实数据库认证、表结构和 MongoDB collection 校验。
- AI 传输：`provider=http` 目前是外部模型适配边界，尚未调用真实远程推理服务。
- MongoDB：当前仅参与依赖探测，非结构化日志、知识片段和长上下文尚未正式落库。
- 客户端：Qt 客户端已接入 HTTP 登录、资产列表、运行监控列表与状态写入、告警列表与处置、维护工单列表、新建/从告警生成/分派/基础流转、AI 结构化诊断入口和 AI 交互审计查询；工单编辑/附件、告警创建/规则/通知联动和 AI 审计分页/导出尚未接入。
- 集成测试：默认 HTTP 冒烟测试覆盖内存仓储；MySQL、Redis、MongoDB 的真实依赖链路需要在独立环境补充集成验证。

## 下一阶段优先级

1. 身份安全深化：在现有密码哈希边界上实现登录失败锁定、会话刷新、审计事件、密码轮换和种子账号替换流程。
2. 真实依赖集成验证：在现有 schema 版本和部署前预检基础上，增加 MySQL 实库查询、Redis 会话测试、MongoDB 初始化验证和 CI 服务容器。
3. Qt 客户端联机化深化：在现有 HTTP 登录、资产、运行监控列表与状态写入、告警列表与处置、维护工单列表、新建/从告警生成/分派/基础流转、AI 诊断入口和 AI 交互审计查询基础上，继续接入工单编辑/附件、告警创建/规则/通知联动和 AI 审计分页/导出。
4. 外部 AI Provider：实现 HTTP 推理传输、超时重试、脱敏、提示词版本、响应解析和降级审计。
5. 可观测性：补充结构化日志、请求追踪、关键指标、健康分级和运行告警。
6. CI/CD 扩展：在现有基础 CI 上增加 Drogon/vcpkg HTTP 构建、Redis/MySQL/MongoDB 真实依赖集成、数据库脚本校验和发布流水线。

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
- 所有 AI 建议必须可降级、可审计，并保留人工复核。
- 新模块先进入 OpenSpec，再实现仓储、服务、HTTP、测试和文档，最后按模块提交。
