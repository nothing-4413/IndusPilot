# 生产化后端模式

## 构建目标

生产化后端模式使用 Drogon 作为 HTTP 服务运行时，并通过 vcpkg 接入 Redis、MySQL、YAML 等依赖：

```powershell
cmake --preset dev-http
cmake --build --preset dev-http
ctest --preset dev-http
```

生成的后端程序位于：

```powershell
build/dev-http/backend/induspilot-backend.exe
```

## 本地依赖

启动 MySQL、Redis、MongoDB：

```powershell
cd deployment
docker compose up -d
```

默认端口：

- MySQL：`127.0.0.1:3306`
- Redis：`127.0.0.1:6379`
- MongoDB：`127.0.0.1:27017`

## 配置覆盖

后端读取 `config/backend.example.yaml`，并支持环境变量覆盖：

- `INDUSPILOT_SERVER_HOST`
- `INDUSPILOT_SERVER_PORT`
- `INDUSPILOT_LOG_LEVEL`
- `INDUSPILOT_MYSQL_HOST`
- `INDUSPILOT_MYSQL_PORT`
- `INDUSPILOT_MYSQL_DATABASE`
- `INDUSPILOT_MYSQL_USER`
- `INDUSPILOT_MYSQL_PASSWORD`
- `INDUSPILOT_MYSQL_URI`
- `INDUSPILOT_REDIS_HOST`
- `INDUSPILOT_REDIS_PORT`
- `INDUSPILOT_REDIS_PASSWORD`
- `INDUSPILOT_REDIS_DATABASE`
- `INDUSPILOT_REDIS_URI`
- `INDUSPILOT_REDIS_SESSION_STORE`
- `INDUSPILOT_REDIS_SESSION_KEY_PREFIX`
- `INDUSPILOT_REDIS_SESSION_TTL_SECONDS`
- `INDUSPILOT_REPOSITORY_STORE`
- `INDUSPILOT_MONGODB_HOST`
- `INDUSPILOT_MONGODB_PORT`
- `INDUSPILOT_MONGODB_DATABASE`
- `INDUSPILOT_MONGODB_URI`
- `INDUSPILOT_AI_ENABLED`
- `INDUSPILOT_AI_PROVIDER`
- `INDUSPILOT_AI_ENDPOINT`

## 仓储运行时

`storage.repository_store` 支持 `memory` 和 `mysql`。默认 `memory` 用于离线演示和测试；设置为 `mysql` 后，HTTP 运行时会将身份认证、资产、告警、维护工单、运行状态和 AI 交互审计切换到 MySQL 仓储。

## 身份口令边界

内存仓储保留 `admin/admin123`、`operator/operator123`、`maintainer/maintainer123` 作为开发演示口令，并通过显式 `plain:` 兼容格式标识。MySQL 初始化脚本写入 PBKDF2-SHA256 演示哈希，便于本地依赖链路登录验证；生产部署前必须为每个账号生成唯一盐哈希，并补充密码轮换、锁定、审计和最小权限账户治理。

## 当前配置边界

- Redis session 已支持通过 `redis.uri` 接入；`redis.password` 和 `redis.database` 会被解析，但当前连接实现不单独消费这两个字段，如需认证或选择 DB，请把信息嵌入 `redis.uri`。
- MongoDB 当前仅用于 TCP 健康探测；AI 交互审计在 `repository_store=mysql` 时写入 MySQL，尚未写入 MongoDB。
- `ai.enabled`、`ai.provider` 和 `ai.endpoint` 当前驱动健康探测、AI 状态接口、agent 诊断编排和降级提示；`provider=http` 已保留外部模型服务适配边界，当前仍使用本地规则降级。
- `/health` 依赖检查当前只验证 TCP 连通性，不校验认证、schema、表结构或 MongoDB collection。

## Session Store

默认示例配置使用：

```yaml
redis:
  session_store: "memory"
```

这适合离线演示和无 Redis 的本机测试。要验证 Redis-backed session，可先启动 Redis，再覆盖：

```powershell
$env:INDUSPILOT_REDIS_SESSION_STORE="redis"
.\build\dev-http\backend\induspilot-backend.exe config\backend.example.yaml
```

## 部署前预检

在启动生产化后端前，可以先运行离线预检，确认配置文件、MySQL 脚本、schema 版本登记和 compose 依赖定义完整：

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File deployment/preflight.ps1
```

如本机已安装 Docker，并希望校验 compose 配置，可增加：

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File deployment/preflight.ps1 -RequireDocker
```

MySQL 初始化脚本会登记以下版本到 `schema_migrations`：`001_foundation_schema`、`002_seed_identity`、`003_runtime_persistence_schema`。该版本表用于部署核对，不代表后端会在启动时自动迁移数据库。
## HTTP 冒烟测试

CTest 已注册 `induspilot-http-integration-smoke`，覆盖：

- 依赖健康字段返回
- 登录成功路径
- 未认证访问保护路由返回 401
- 权限不足返回 403
- 请求体缺字段返回 400
- 资源不存在返回 404

手动运行：

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File backend/tests/http_integration_smoke.ps1 `
  -BackendExe build/dev-http/backend/induspilot-backend.exe `
  -ConfigPath config/backend.example.yaml
```

验证 Redis-backed session：

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File backend/tests/http_integration_smoke.ps1 `
  -BackendExe build/dev-http/backend/induspilot-backend.exe `
  -ConfigPath config/backend.example.yaml `
  -SessionStore redis
```

## 生产注意事项

- 开发口令和 MySQL 演示哈希仍是骨架数据，生产必须替换为唯一盐哈希、密码轮换、登录失败锁定和审计策略。
- MySQL 仓储已经覆盖 identity、asset、alert、work-order、runtime-state 和 AI interaction；生产部署前需执行 `database/mysql/001_foundation_schema.sql`、`002_seed_identity.sql`、`003_runtime_persistence_schema.sql`，并确认 `schema_migrations` 已登记对应版本。
- Redis session 已支持配置化接入，后续需要补充连接失败降级策略和监控指标。
- 当前 MongoDB 仅做依赖探测，后续可用于长日志、知识片段或非结构化诊断上下文。
- 当前 HTTP 冒烟测试默认使用内存仓储；`deployment/preflight.ps1` 覆盖离线部署基线，MySQL/Redis/MongoDB 的真实连通和认证仍需要在部署环境中补充集成验证。
