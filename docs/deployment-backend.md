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
- `INDUSPILOT_SECURITY_PASSWORD_MIN_LENGTH`
- `INDUSPILOT_SECURITY_PASSWORD_ITERATIONS`
- `INDUSPILOT_SECURITY_PRODUCTION_MODE`
- `INDUSPILOT_SECURITY_ALLOW_SEED_CREDENTIALS`

## 启动与健康检查

后端启动时先校验静态配置。配置错误会返回退出码 `78`，且不会启动 HTTP listener；MySQL、Redis 或 required AI 暂时不可用不会导致进程退出，而会让 readiness 返回 `503`。

配置文件必须存在且可读取。整数和布尔环境变量必须完整匹配合法值；未知 section、字段、错误层级或损坏行也会被拒绝，不会静默回退默认值。启动失败时优先查看 HTTP listener 启动前的进程 stderr。

编排系统应分别使用以下 endpoint：

- `/health/live`：只判断进程是否已运行，不执行外部依赖探测，返回 `200` 才继续保留实例。
- `/health/ready`：判断 required 依赖，全部可用返回 `200`，否则返回 `503`；响应包含每个依赖的 `required`、`available`、`checked` 和 `reason`，以及探测 metadata。
- `/health/startup`：判断配置校验和初始化是否完成；有效启动后返回 `200`。
- `/health`：兼容旧客户端，继续返回 `200` 以及 `service`、`dependencies`、`warnings` 字段。

readiness 探测使用 `INDUSPILOT_READINESS_PROBE_TIMEOUT_MS` 限制 DNS、TCP connect 以及 MongoDB authenticated `ping` 的单轮 deadline，并在 `INDUSPILOT_READINESS_PROBE_CACHE_MS` 内复用结果。MongoDB AI 仓储 client 也复用该值限制 server selection/connect，避免依赖异常后的首次 AI 操作等待驱动默认长超时。多个并发 readiness 请求会合并为一轮探测；缓存过期后下一次 readiness 请求会重新探测，依赖恢复后可自动回到 `200`。readiness data 还提供 `probeInProgress`、`probeCount`、`failureCount`、`recoveryCount`、`lastProbeDurationMs` 和 `lastProbeAtUnixMs`。

选择 MongoDB AI 仓储时，必须提供非空 `mongodb.database`；若 `mongodb.uri` 为空，则还必须提供合法的 `mongodb.host` 和 `mongodb.port`，HTTP 组合根会使用 `mongodb://host:port` fallback，与 readiness 探测保持一致。临时认证失败、网络不可用或 `ping` 返回非正 `ok` 不会阻止 HTTP listener 启动；`/health/live` 仍返回 `200`，`/health/ready` 返回 `503` 并标记 MongoDB required/unavailable。readiness reason 会限制长度并移除 MongoDB URI，响应不得包含 URI、用户名或密码。MongoDB 仓储会在首次 AI 读写前重试尚未完成的索引协调；数据库权限不足、重复 `interactionCode` 或不兼容索引等结构性协调错误仍会阻止正常启动或操作。
认证成功但业务库权限不足是结构性配置错误，不等同于临时网络故障：启动期索引协调会 fail closed 并返回非零结果，错误诊断同样不得包含 MongoDB URI、用户名或密码。`ping` 成功只表示认证连接可用，不代表账号拥有 AI 集合的索引/写权限。

本地或反向代理可用以下命令确认状态：

```powershell
Invoke-WebRequest http://127.0.0.1:8080/health/live
Invoke-WebRequest http://127.0.0.1:8080/health/ready
Invoke-WebRequest http://127.0.0.1:8080/health/startup
```

`deployment/docker-compose.yml` 中的 healthcheck 只表示 MySQL、Redis、MongoDB 容器自身可接受连接；它不能替代后端 `/health/ready`。MongoDB 仅在 `storage.ai_interaction_store=mongodb` 时成为 readiness required 条件。

## 优雅停机

HTTP runtime 已将 `SIGTERM` 和 `SIGINT` 绑定到同一个 shutdown coordinator。收到信号后，实例按以下顺序处理：

1. 停止接受新的业务请求；新业务请求返回 `503` 和 `SERVER_DRAINING`。
2. Application 进入 draining；此时 `/health/ready` 返回 `503`，`/health/live` 仍返回 `200`，让编排器完成摘流。
3. 在 `shutdown.drain_timeout_ms` deadline 内等待已通过请求 gate 的请求完成。
4. 调用 Drogon `quit()`，释放 listener 和运行时资源，进程正常返回 `0`。

健康检查和 `/metrics` 在 draining 阶段仍可访问，以便观察状态。deadline 到期不会强制取消业务线程，但会记录剩余请求数量并让 HTTP runtime 退出；部署平台的 termination grace period 应大于该 deadline。listener 预检失败返回退出码 `69`，静态配置错误仍返回 `78`。

## 仓储运行时

`storage.repository_store` 支持 `memory` 和 `mysql`，控制身份认证、资产、告警、维护工单、运行状态和操作审计等事务型数据。`storage.ai_interaction_store` 独立支持 `memory`、`mysql` 和 `mongodb`，默认 `memory`；选择 `mongodb` 时需使用 `dev-http-mongodb` preset 或以 `INDUSPILOT_WITH_MONGODB=ON` 和 vcpkg `mongodb` feature 构建。MongoDB 写入 `ai_interactions` 集合，按 `interactionCode` upsert，读取保持按关联对象过滤并按创建时间倒序。

选择 MongoDB 仓储时，后端在认证探测成功后会协调 `ai_interactions` 的 `interactionCode` 唯一索引与关联对象查询索引，因此已有 MongoDB 数据卷不依赖 Docker 首次初始化脚本。若认证/网络暂不可用，协调会延迟到首次 AI 读写并重试；若创建唯一索引因历史重复 `interactionCode` 失败，后端会拒绝启动。应先备份受影响文档、为每个重复键保留一个权威记录或合并其内容、删除其余重复记录，再重试启动；不得通过删除唯一索引绕过该失败。

`/metrics` 会输出 `induspilot_mongodb_ai_interaction_operations_total`、`induspilot_mongodb_ai_interaction_errors_total`、`induspilot_mongodb_ai_interaction_duration_ms_sum` 和 `induspilot_mongodb_ai_interaction_duration_ms_count`，按固定的 `reconcile`、`read`、`write` 操作标签统计。指标不包含交互编号、查询条件、凭据或异常文本。

## 身份口令边界

内存仓储保留 `admin/admin123`、`operator/operator123`、`maintainer/maintainer123` 作为开发演示口令，并通过显式 `plain:` 兼容格式标识。MySQL 初始化脚本写入 PBKDF2-SHA256 演示哈希并由 `012_seed_account_governance` 标记为待轮换；生产模式拒绝这类凭据，登录不会签发 session。只有本地示例配置显式启用 `security.allow_seed_credentials`，生产部署必须保持关闭。

密码策略由 `security.password_min_length` 和 `security.password_iterations` 控制，也可通过环境变量覆盖。最小长度允许范围为 `8..1024`，PBKDF2 迭代次数允许范围为 `100000..1000000`；配置不满足范围时，后端在 listener 启动前以退出码 `78` 失败。新密码始终写入随机盐 PBKDF2-SHA256 哈希，旧的 `plain:` 格式只用于开发兼容读取。

认证用户可调用 `POST /api/v1/auth/password` 轮换自己的密码，请求体为 `{"currentPassword":"...","newPassword":"..."}`，需要当前 Bearer session。当前密码错误返回 `401`，新密码策略不合规返回 `400`，存储失败返回 `503`；响应、请求日志和审计事件不包含明文密码或密码哈希。密码轮换后当前 session 保持有效。

生产部署流程应在实例投入流量前使用 `deployment/rotate_seed_credentials.ps1` 为每个种子账号生成唯一盐 PBKDF2 哈希、清除待轮换标记并递增凭据版本，再确认 `security.allow_seed_credentials` 关闭和 `auth.login.seed_rotation_required` 不再产生。轮换工具不提供 HTTP bootstrap 接口，避免公开演示口令成为首个管理员写入者；新凭据和哈希不会写入日志。

生产环境应通过 `INDUSPILOT_SECURITY_PRODUCTION_MODE=true` 显式启用启动期治理，并保持 `INDUSPILOT_SECURITY_ALLOW_SEED_CREDENTIALS=false`。启用后，MySQL 仓储拒绝空值或 `change-me*` 示例应用密码，且会在 HTTP listener 启动前失败。

## 当前配置边界

- Redis session 已支持通过 `redis.uri` 接入；`redis.password` 和 `redis.database` 会被解析，但当前连接实现不单独消费这两个字段，如需认证或选择 DB，请把信息嵌入 `redis.uri`。
- MongoDB 只承载 AI 交互文档；身份、资产、告警、工单、运行状态和操作审计哈希链仍由 MySQL 主存储管理。选择 MongoDB AI 仓储时，连接失败会导致 AI 交互读写返回 `503 DEPENDENCY_UNAVAILABLE`，不会伪造已持久化成功；readiness 同时将 MongoDB 标为 required。
- `ai.enabled`、`ai.provider`、`ai.endpoint`、`ai.timeoutMs`、`ai.maxContextItems` 和 `ai.storeInteractionRecords` 驱动健康探测、AI 状态接口、agent 诊断编排、HTTP provider 推理传输和交互审计记录策略；非 Drogon 构建或 HTTP 调用失败时仍使用本地规则降级。
- `/health` 依赖检查会对 MySQL、Redis 和 HTTP AI 使用 TCP 探测；选择 MongoDB AI 仓储时会执行带认证和超时边界的 MongoDB `ping` 命令。MongoDB collection/索引由启动期协调、首次操作重试和 dependency/runtime profile smoke 覆盖。

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

真实 `deployment/.env` 已配置后，生产发布前还应运行下列检查。它只报告变量名，不输出秘密值：

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File deployment/preflight.ps1 -RequireDocker -RequireProductionSecrets
```

MySQL 初始化脚本会登记以下版本到 `schema_migrations`：`001_foundation_schema`、`002_seed_identity`、`003_runtime_persistence_schema`、`004_work_order_attachments_schema`、`005_alert_rules_notifications_schema`、`006_alert_notification_delivery_schema`、`007_operation_audit_events_schema`、`008_operation_audit_export_permission`、`009_operation_audit_integrity_schema`、`010_redact_legacy_login_audit_tokens`、`011_credential_version`、`012_seed_account_governance`、`013_notification_delivery_queue`、`014_migration_integrity`。该版本表用于部署核对；后端不会在启动时自动迁移数据库。

CI 的依赖服务和真实后端运行时 profile 会把 `INDUSPILOT_MYSQL_DATABASE` 覆盖为 `induspilot_ci_custom_db`，并通过迁移、CRUD 与 HTTP smoke 验证整个链路。生产仍应显式设置自身数据库名；该 CI 名称不应直接用于部署环境。

生产或升级环境应使用统一迁移入口。它会按编号应用待执行脚本，拒绝未知版本和版本跳跃，并在每个脚本完成后校验版本登记：

```bash
MYSQL_HOST=127.0.0.1 MYSQL_PORT=3306 MYSQL_USER=root MYSQL_PWD='your-root-password' \
  MYSQL_DATABASE=induspilot bash database/mysql/migrate.sh
```

`MYSQL_DATABASE` 可替换为部署使用的合法数据库标识符；迁移脚本通过 `MYSQL_PWD` 接收密码，避免将密码放入 mysql 命令行参数。Runner 在同一个 MySQL 会话中持有 advisory lock，并为每个已登记迁移验证 SHA-256 checksum；checksum 不匹配或锁已被占用时必须停止排查，不能绕过后继续执行。不支持自动回滚，失败后应先修复数据库状态再重新执行。
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

- 开发口令和 MySQL 演示哈希仍是骨架数据，生产必须替换为唯一盐哈希，并通过密码轮换接口完成首轮治理；登录失败锁定和审计策略已有基础实现，但仍需结合部署规模验证保留周期和告警策略。
- MySQL 仓储已经覆盖 identity、asset、alert、work-order、runtime-state、operation audit，并可选承载 AI interaction；生产部署前需通过 `database/mysql/migrate.sh` 执行受控迁移，并确认 `schema_migrations` 已登记对应版本。
- Redis session 已支持配置化接入，后续需要补充连接失败降级策略和监控指标。
- MongoDB 已用于 AI interaction 文档；长日志、知识片段和非结构化诊断上下文仍需独立设计，不应绕过现有 MySQL 事务边界和审计链。
- 当前 HTTP 冒烟测试默认使用内存仓储；`deployment/preflight.ps1` 覆盖离线部署基线，CI dependency smoke 已覆盖 MySQL/Redis/MongoDB 真实启动、认证、MySQL 核心业务 CRUD、Redis 数据结构读写和 MongoDB 文档 CRUD。
## 真实依赖冒烟测试

CI 会使用 `deployment/docker-compose.yml` 启动 MySQL、Redis 和 MongoDB，并运行 `backend/tests/dependency_services_smoke.sh`。该测试会先运行迁移入口的 fake-client 回归测试，再使用同一迁移入口重复执行 MySQL 迁移，验证幂等性、顺序和 `schema_migrations`，执行 `database/mysql/integration/real_crud_smoke.sql` 覆盖真实 MySQL 核心业务 CRUD，验证 Redis 鉴权 `PING`、key/value、TTL、counter 和 hash 读写，并加载 MongoDB 初始化脚本后执行 `ping` 与 `database/mongodb/integration/real_crud_smoke.js` 文档 CRUD。

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
