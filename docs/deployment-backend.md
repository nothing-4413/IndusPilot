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
copy .env.example .env
# 编辑 .env，替换所有 change-me-* 密钥
docker compose up -d
```

默认端口仅绑定到 `127.0.0.1`，如需远程访问请在 `.env` 中显式修改 `*_BIND`：

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
- `INDUSPILOT_AI_REQUIRED`
- `INDUSPILOT_AI_PROVIDER`
- `INDUSPILOT_AI_ENDPOINT`
- `INDUSPILOT_AI_TIMEOUT_MS`
- `INDUSPILOT_AI_MAX_CONTEXT_ITEMS`
- `INDUSPILOT_AI_STORE_INTERACTION_RECORDS`
- `INDUSPILOT_READINESS_PROBE_TIMEOUT_MS`
- `INDUSPILOT_READINESS_PROBE_CACHE_MS`

## 启动与健康检查

后端启动时先校验静态配置。配置错误会返回退出码 `78`，且不会启动 HTTP listener；MySQL、Redis 或 required AI 暂时不可用不会导致进程退出，而会让 readiness 返回 `503`。

配置文件必须存在且可读取。整数和布尔环境变量必须完整匹配合法值；未知 section、字段、错误层级或损坏行也会被拒绝，不会静默回退默认值。启动失败时优先查看 HTTP listener 启动前的进程 stderr。

编排系统应分别使用以下 endpoint：

- `/health/live`：只判断进程是否已运行，不执行外部依赖探测，返回 `200` 才继续保留实例。
- `/health/ready`：判断 required 依赖，全部可用返回 `200`，否则返回 `503`；响应包含每个依赖的 `required`、`available`、`checked` 和 `reason`，以及探测 metadata。
- `/health/startup`：判断配置校验和初始化是否完成；有效启动后返回 `200`。
- `/health`：兼容旧客户端，继续返回 `200` 以及 `service`、`dependencies`、`warnings` 字段。

readiness 探测使用 `INDUSPILOT_READINESS_PROBE_TIMEOUT_MS` 限制 DNS 和 TCP connect 的单轮 deadline，并在 `INDUSPILOT_READINESS_PROBE_CACHE_MS` 内复用结果。多个并发 readiness 请求会合并为一轮探测；缓存过期后下一次 readiness 请求会重新探测，依赖恢复后可自动回到 `200`。readiness data 还提供 `probeInProgress`、`probeCount`、`failureCount`、`recoveryCount`、`lastProbeDurationMs` 和 `lastProbeAtUnixMs`。

本地或反向代理可用以下命令确认状态：

```powershell
Invoke-WebRequest http://127.0.0.1:8080/health/live
Invoke-WebRequest http://127.0.0.1:8080/health/ready
Invoke-WebRequest http://127.0.0.1:8080/health/startup
```

`deployment/docker-compose.yml` 中的 healthcheck 只表示 MySQL、Redis、MongoDB 容器自身可接受连接；它不能替代后端 `/health/ready`。MongoDB 当前没有业务仓储接入，因此即使 compose 健康，也不会成为核心 readiness 条件。

## 优雅停机

HTTP runtime 已将 `SIGTERM` 和 `SIGINT` 绑定到同一个 shutdown coordinator。收到信号后，实例按以下顺序处理：

1. 停止接受新的业务请求；新业务请求返回 `503` 和 `SERVER_DRAINING`。
2. Application 进入 draining；此时 `/health/ready` 返回 `503`，`/health/live` 仍返回 `200`，让编排器完成摘流。
3. 等待已通过请求 gate 的请求完成。
4. 调用 Drogon `quit()`，释放 listener 和运行时资源，进程正常返回 `0`。

健康检查和 `/metrics` 在 draining 阶段仍可访问，以便观察状态。当前策略不强制中断超时业务请求；如部署平台的 termination grace period 到期，平台可能直接终止进程。listener 预检失败返回退出码 `69`，静态配置错误仍返回 `78`。

## 仓储运行时

`storage.repository_store` 支持 `memory` 和 `mysql`。默认 `memory` 用于离线演示和测试；设置为 `mysql` 后，HTTP 运行时会将身份认证、资产、告警、维护工单、运行状态和 AI 交互审计切换到 MySQL 仓储。

## 身份口令边界

内存仓储保留 `admin/admin123`、`operator/operator123`、`maintainer/maintainer123` 作为开发演示口令，并通过显式 `plain:` 兼容格式标识。MySQL 初始化脚本写入 PBKDF2-SHA256 演示哈希，便于本地依赖链路登录验证；生产部署前必须为每个账号生成唯一盐哈希，并补充密码轮换、锁定、审计和最小权限账户治理。

## 当前配置边界

- Redis session 已支持通过 `redis.uri` 接入；`redis.password` 和 `redis.database` 会被解析，但当前连接实现不单独消费这两个字段，如需认证或选择 DB，请把信息嵌入 `redis.uri`。
- MongoDB 当前尚未接入后端业务仓储；运行时只做 TCP 健康探测，CI dependency smoke 会验证初始化集合、索引和文档 CRUD。AI 交互审计在 `repository_store=mysql` 时写入 MySQL。
- `ai.enabled`、`ai.provider`、`ai.endpoint`、`ai.timeoutMs`、`ai.maxContextItems` 和 `ai.storeInteractionRecords` 驱动健康探测、AI 状态接口、agent 诊断编排、HTTP provider 推理传输和交互审计记录策略；非 Drogon 构建或 HTTP 调用失败时仍使用本地规则降级。
- `/health` 依赖检查当前只验证 TCP 连通性；认证、schema、表结构、Redis 读写和 MongoDB collection/索引由 dependency smoke 与部署预检覆盖。

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

MySQL 初始化脚本会登记以下版本到 `schema_migrations`：`001_foundation_schema`、`002_seed_identity`、`003_runtime_persistence_schema`、`004_work_order_attachments_schema`、`005_alert_rules_notifications_schema`、`006_alert_notification_delivery_schema`、`007_operation_audit_events_schema`、`008_operation_audit_export_permission`、`009_operation_audit_integrity_schema`。该版本表用于部署核对，不代表后端会在启动时自动迁移数据库。
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

验证 Redis-backed session 或 MySQL 仓储时，可使用同一 HTTP smoke 脚本传入运行时 profile。默认 CTest 不依赖外部服务；启动 compose 依赖并执行数据库初始化后，可手动切换到真实依赖：

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File backend/tests/http_integration_smoke.ps1 `
  -BackendExe build/dev-http/backend/induspilot-backend.exe `
  -ConfigPath config/backend.example.yaml `
  -RepositoryStore mysql `
  -SessionStore redis `
  -MySqlUri "host=127.0.0.1 port=3306 dbname=induspilot user=induspilot password=ci-app-password" `
  -RedisUri "tcp://:ci-redis-password@127.0.0.1:6379/0" `
  -MongoDbUri "mongodb://induspilot:ci-mongodb-password@127.0.0.1:27017/admin"
```

## 生产注意事项

- 开发口令和 MySQL 演示哈希仍是骨架数据，生产必须替换为唯一盐哈希、密码轮换、登录失败锁定和审计策略。
- MySQL 仓储已经覆盖 identity、asset、alert、work-order、runtime-state、AI interaction 和 operation audit；生产部署前需执行 `database/mysql/001_foundation_schema.sql` 到 `009_operation_audit_integrity_schema.sql`，并确认 `schema_migrations` 已登记对应版本。
- Redis session 已支持配置化接入，后续需要补充连接失败降级策略和监控指标。
- 当前 MongoDB 仅做依赖探测，后续可用于长日志、知识片段或非结构化诊断上下文。
- 当前 HTTP 冒烟测试默认使用内存仓储；`deployment/preflight.ps1` 覆盖离线部署基线，CI dependency smoke 已覆盖 MySQL/Redis/MongoDB 真实启动、认证、MySQL 核心业务 CRUD、Redis 数据结构读写和 MongoDB 文档 CRUD。
## 真实依赖冒烟测试

CI 会使用 `deployment/docker-compose.yml` 启动 MySQL、Redis 和 MongoDB，并运行 `backend/tests/dependency_services_smoke.sh`。该测试会重复执行 MySQL 迁移脚本以验证幂等性，检查 `schema_migrations`，执行 `database/mysql/integration/real_crud_smoke.sql` 覆盖真实 MySQL 核心业务 CRUD，验证 Redis 鉴权 `PING`、key/value、TTL、counter 和 hash 读写，并加载 MongoDB 初始化脚本后执行 `ping` 与 `database/mongodb/integration/real_crud_smoke.js` 文档 CRUD。

本地已安装 Docker 且已经构建 `dev-http` 后，可运行真实运行时 profile 验收脚本。脚本会读取 `deployment/.env`，拒绝 `change-me-*` 示例密钥，并可选启动依赖、运行 dependency smoke、执行 MySQL 仓储 + Redis session HTTP smoke：

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File backend/tests/http_runtime_profile_smoke.ps1 `
  -StartDependencies `
  -RunDependencySmoke `
  -StopDependencies
```

本地已安装 Docker 时也可只运行依赖 smoke：
```powershell
cd deployment
copy .env.example .env
# 编辑 .env 后启动依赖
docker compose up -d --wait
cd ..
bash backend/tests/dependency_services_smoke.sh
```
