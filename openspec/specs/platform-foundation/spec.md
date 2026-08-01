# platform-foundation Specification

## Purpose
定义后端运行时、配置加载、依赖健康检查、统一 API 响应、模块化边界和生产化构建基础，支撑各业务模块独立演进。
## Requirements
### Requirement: Application foundation boundaries
The system SHALL define separate foundation boundaries for the Qt client, C++ backend, data stores, and AI integration layer.

#### Scenario: Foundation boundaries are visible
- **WHEN** a developer inspects the project structure
- **THEN** the client, backend, database, deployment, and AI integration areas are identifiable as separate concerns

### Requirement: Configuration and environment baseline
The system SHALL provide a configuration baseline for local development, service startup, database connections, and AI integration settings.

#### Scenario: Local configuration is prepared
- **WHEN** a developer starts the project in a local environment
- **THEN** required configurable values are documented and separated from source code defaults

### Requirement: Observability baseline
The system SHALL define baseline logging, error reporting, and health-check behavior for production-style operation.

#### Scenario: Service health is inspected
- **WHEN** an operator checks backend health
- **THEN** the system reports whether core dependencies are reachable

### Requirement: Backend service runtime

The system SHALL expose a configurable backend service runtime instead of relying only on in-process module calls.

#### Scenario: Backend starts as a service
- **WHEN** the backend starts with valid configuration
- **THEN** it binds to the configured host and port
- **AND** exposes health and module API endpoints

#### Scenario: Backend reports dependency health
- **WHEN** a health request is received
- **THEN** the response includes MySQL, Redis, MongoDB, and AI adapter availability

### Requirement: Configuration management

The system SHALL load backend configuration from structured files with environment variable overrides for deployable environments.

#### Scenario: Environment overrides service configuration
- **WHEN** an environment variable overrides a configured dependency value
- **THEN** the backend uses the environment value during startup and health checks

### Requirement: Protected API operations

The system SHALL require valid sessions and permissions before protected industrial operations can be executed.

#### Scenario: Protected route rejects missing session
- **WHEN** a request to a protected route omits a valid session token
- **THEN** the backend returns an authentication error without executing the operation

#### Scenario: Protected route rejects missing permission
- **WHEN** an authenticated user lacks the permission required by a route
- **THEN** the backend returns an authorization error without executing the operation

### Requirement: CI 启动真实依赖执行冒烟测试
系统 SHALL 在 CI 中启动项目 compose 定义的 MySQL、Redis 和 MongoDB，并执行最小真实依赖冒烟验证。

#### Scenario: 真实依赖编排启动

- **GIVEN** GitHub Actions 运行主分支或拉取请求 CI
- **WHEN** 执行依赖集成测试 job
- **THEN** CI SHALL 使用 `deployment/docker-compose.yml` 和 CI 专用 `.env` 启动 MySQL、Redis、MongoDB
- **AND** job 完成后 SHALL 清理 compose 容器和卷

#### Scenario: MySQL 迁移可在真实数据库重复执行

- **GIVEN** MySQL 容器已启动并完成初始化
- **WHEN** 依赖冒烟测试重复执行 `database/mysql/*.sql`
- **THEN** 迁移 SHALL 成功完成并能查询到 `schema_migrations` 与审计哈希列

#### Scenario: Redis 和 MongoDB 最小可用验证

- **GIVEN** Redis 和 MongoDB 容器已启动
- **WHEN** 依赖冒烟测试运行
- **THEN** Redis SHALL 通过密码执行 `PING`
- **AND** MongoDB SHALL 加载初始化脚本并通过 `ping` 命令

### Requirement: HTTP 响应回传请求追踪编号
系统 SHALL 为所有 Drogon HTTP 响应回传可用于日志和审计关联的追踪编号。

#### Scenario: 客户端传入 X-Request-Id

- **GIVEN** HTTP 请求包含 `X-Request-Id`
- **WHEN** 后端返回响应
- **THEN** 响应头 SHALL 包含同值的 `X-Trace-Id` 和 `X-Request-Id`

#### Scenario: 客户端传入 X-Trace-Id

- **GIVEN** HTTP 请求包含 `X-Trace-Id`
- **WHEN** 后端写入结构化请求日志或操作审计
- **THEN** 日志与审计 SHALL 使用该追踪编号

#### Scenario: 客户端未传入追踪头

- **GIVEN** HTTP 请求未包含 `X-Trace-Id` 或 `X-Request-Id`
- **WHEN** 后端处理请求
- **THEN** 系统 SHALL 生成 `trace-<timestamp>-<sequence>` 格式追踪编号并在响应头回传

### Requirement: 工业闭环演示脚本
系统 SHALL 提供可复现的工业告警到维护工单闭环演示脚本，用于展示核心模块协同能力。

#### Scenario: 运行端到端演示

- **GIVEN** Drogon HTTP 后端已启动
- **AND** 默认演示账号可登录
- **WHEN** 用户运行闭环演示脚本
- **THEN** 脚本 SHALL 创建演示资产和运行状态
- **AND** 脚本 SHALL 创建关键告警并派发通知
- **AND** 脚本 SHALL 调用 AI 诊断接口
- **AND** 脚本 SHALL 创建并关闭维护工单
- **AND** 脚本 SHALL 关闭告警并导出审计 CSV

#### Scenario: 演示输出可追溯

- **GIVEN** 演示脚本执行成功
- **WHEN** 用户查看输出目录
- **THEN** 输出 SHALL 包含本次运行的 summary JSON
- **AND** 输出 SHALL 包含关键业务步骤响应文件
- **AND** 输出 SHALL 包含审计 CSV 文件

### Requirement: 操作审计哈希链

系统 SHALL 为新写入的操作审计事件生成基于上一条审计事件哈希的 SHA-256 哈希链。

#### Scenario: 记录首条审计事件

- **GIVEN** 操作审计仓储中没有历史事件
- **WHEN** 后端记录一条新的操作审计事件
- **THEN** 该事件的 `previousHash` SHALL be `genesis`
- **AND** 该事件的 `eventHash` SHALL be non-empty

#### Scenario: 记录后续审计事件

- **GIVEN** 操作审计仓储中已经存在带 `eventHash` 的最新事件
- **WHEN** 后端记录下一条操作审计事件
- **THEN** 新事件的 `previousHash` SHALL equal 最新历史事件的 `eventHash`
- **AND** 新事件的 `eventHash` SHALL be generated from canonical audit payload and `previousHash`

### Requirement: 操作审计完整性校验接口

系统 SHALL 提供受 `audit:read` 权限保护的审计完整性校验接口。

#### Scenario: 管理员校验审计链

- **GIVEN** 管理员已登录并具备 `audit:read` 权限
- **WHEN** 管理员调用 `GET /api/v1/audit/integrity`
- **THEN** 后端 SHALL return 校验结果、记录总数、断点事件编号和最新哈希

#### Scenario: 无审计读取权限用户被拒绝

- **GIVEN** 操作员已登录但不具备 `audit:read` 权限
- **WHEN** 操作员调用 `GET /api/v1/audit/integrity`
- **THEN** 后端 SHALL 返回 HTTP 403

#### Scenario: 审计事件被篡改

- **GIVEN** 已存在一组审计事件
- **WHEN** 任意事件内容或哈希链字段被修改导致重算结果不一致
- **THEN** 完整性校验结果 SHALL mark `verified` as false
- **AND** 响应 SHALL include first broken event id

### Requirement: Qt 客户端审计完整性展示

Qt 客户端 SHALL 在操作审计页面展示审计哈希链完整性状态。

#### Scenario: 刷新操作审计时展示完整性状态

- **GIVEN** 用户打开“操作审计”页面并已连接后端
- **WHEN** 客户端刷新审计列表
- **THEN** 客户端 SHALL call `GET /api/v1/audit/integrity`
- **AND** 页面 SHALL 显示中文完整性状态

#### Scenario: 用户手动校验审计链

- **GIVEN** 用户位于“操作审计”页面
- **WHEN** 用户点击“校验”按钮
- **THEN** 客户端 SHALL refresh the integrity status without exporting audit data

#### Scenario: AI provider configuration is loaded
- **WHEN** configuration contains AI provider settings or matching environment overrides
- **THEN** the runtime exposes the selected provider and endpoint to the AI diagnosis module

### Requirement: MySQL 健康探测遵循连接 URI 优先级

系统 SHALL 在 MySQL 健康探测中使用与 MySQL 仓储一致的连接优先级：`mysql.uri` 非空时优先解析 URI，URI 为空时使用 `mysql.host` 和 `mysql.port`。

#### Scenario: 配置了 MySQL URI

- **GIVEN** `mysql.uri` 非空
- **WHEN** 后端生成依赖健康状态
- **THEN** MySQL TCP 探测 SHALL use the host and port parsed from `mysql.uri`

#### Scenario: 未配置 MySQL URI

- **GIVEN** `mysql.uri` 为空
- **WHEN** 后端生成依赖健康状态
- **THEN** MySQL TCP 探测 SHALL use `mysql.host` and `mysql.port`

### Requirement: Qt 客户端 AI 审计导出

Qt 客户端 SHALL allow operators to export the currently displayed AI interaction audit table as a CSV file.

#### Scenario: 导出当前审计表格

- **GIVEN** AI 交互审计表格中存在记录
- **WHEN** 操作员选择导出审计并确认保存路径
- **THEN** 客户端 SHALL write a UTF-8 CSV file containing the table headers and rows
- **AND** 客户端 SHALL escape CSV fields containing commas, quotes or new lines
- **AND** 客户端 SHALL show a Chinese success message containing the saved path

#### Scenario: 审计表为空时阻止导出

- **GIVEN** AI 交互审计表格中没有记录
- **WHEN** 操作员选择导出审计
- **THEN** 客户端 SHALL not create a file
- **AND** 客户端 SHALL show a Chinese message asking the operator to refresh or query audit records first

### Requirement: AI 交互审计分页查询

系统 SHALL 支持对 AI 交互审计记录进行兼容式分页查询。

#### Scenario: 旧版审计查询保持数组响应

- **WHEN** 已认证用户调用 `GET /api/v1/ai/interactions` 且未传入 `limit` 或 `offset`
- **THEN** 后端 SHALL 返回既有数组格式的 `data` 字段
- **AND** 既有 Qt 客户端和脚本 SHALL 不需要修改即可继续读取审计记录

#### Scenario: 分页审计查询返回总数和当前页

- **WHEN** 已认证用户调用 `GET /api/v1/ai/interactions?limit=10&offset=0`
- **THEN** 后端 SHALL 返回对象格式的 `data` 字段
- **AND** `data.items` SHALL contain the current page of AI interaction records
- **AND** `data.total`, `data.limit`, and `data.offset` SHALL describe the full filtered result set and page request

#### Scenario: 无效分页参数被拒绝

- **WHEN** 已认证用户传入小于 1 或大于 100 的 `limit`，或传入负数 `offset`
- **THEN** 后端 SHALL return HTTP 400 with a Chinese validation message

### Requirement: Qt 客户端 AI 审计分页控件

Qt 客户端 SHALL allow operators to browse AI interaction audit records by page.

#### Scenario: 操作员翻页查看审计

- **GIVEN** AI 辅助页面已经打开
- **WHEN** 操作员点击上一页或下一页
- **THEN** Qt 客户端 SHALL request the backend with the selected `limit` and calculated `offset`
- **AND** the audit table SHALL display only the returned page
- **AND** the page summary SHALL show the visible range and total count in Chinese

### Requirement: Qt client AI diagnosis synchronization
The system SHALL allow the Qt client to submit structured AI diagnosis context to the backend after login.

#### Scenario: Diagnosis request is submitted from Qt client
- **WHEN** an authenticated user fills diagnosis context and runs AI diagnosis
- **THEN** the client calls the backend AI diagnosis endpoint with related object, prompt, and context fields

### Requirement: Qt client AI diagnosis presentation
The system SHALL present backend AI diagnosis results in a structured operator-readable format.

#### Scenario: Diagnosis result is returned
- **WHEN** the backend returns summary, risk level, possible causes, recommended actions, and review metadata
- **THEN** the Qt client displays those fields without requiring the operator to inspect raw JSON

### Requirement: Qt client AI diagnosis fallback
The system SHALL keep AI diagnosis non-blocking when backend or provider calls are unavailable.

#### Scenario: Diagnosis request cannot be completed online
- **WHEN** the Qt client cannot obtain an online AI diagnosis result
- **THEN** it shows a local fallback message and keeps core alert and work-order workflows available

### Requirement: Qt client AI interaction audit synchronization
The system SHALL allow the Qt client to load AI interaction audit records from the backend after login.

#### Scenario: AI interactions are loaded for a related object
- **WHEN** an authenticated user refreshes AI interaction history for a related type and related id
- **THEN** the client calls the backend AI interactions endpoint and displays matching audit rows

### Requirement: Qt client AI interaction audit refresh
The system SHALL refresh AI interaction audit rows after a new AI diagnosis is submitted.

#### Scenario: Diagnosis creates an audit record
- **WHEN** the Qt client receives a diagnosis response
- **THEN** it refreshes the AI interaction history table for the current related object

### Requirement: Qt client AI interaction audit fallback
The system SHALL keep local AI interaction demo rows available when backend audit calls fail.

#### Scenario: Backend AI interaction request fails
- **WHEN** the Qt client cannot load AI interaction history
- **THEN** it keeps local demo rows visible and shows an offline fallback message

### Requirement: Qt client alert lifecycle actions
The system SHALL allow the Qt client to submit lifecycle actions for selected alerts through the backend.

#### Scenario: Operator acknowledges a selected alert
- **WHEN** an authenticated operator selects an alert and acknowledges it
- **THEN** the client calls the backend acknowledge endpoint and refreshes the alert list

#### Scenario: Operator resolves or closes a selected alert
- **WHEN** an authenticated operator selects an alert and resolves or closes it
- **THEN** the client calls the corresponding backend endpoint and refreshes the alert list

### Requirement: Qt client alert assignment
The system SHALL allow the Qt client to assign a selected alert through the backend.

#### Scenario: Operator assigns a selected alert
- **WHEN** an authenticated operator selects an alert and submits an assignee
- **THEN** the client calls the backend alert assignment endpoint and refreshes the alert list

### Requirement: Qt client alert action fallback
The system SHALL keep alert actions non-blocking when backend calls fail.

#### Scenario: Backend alert action request fails
- **WHEN** the Qt client cannot submit an alert action
- **THEN** it keeps the visible alert list available and shows a Chinese failure message

### Requirement: Qt 客户端告警创建

Qt 客户端 SHALL provide an authenticated workflow for operators to create alerts through the backend.

#### Scenario: 创建告警后刷新列表

- **GIVEN** 操作员已通过 Qt 客户端登录后端
- **WHEN** 操作员填写告警编号、设备编号、级别、状态、标题和负责人并提交
- **THEN** 客户端 SHALL call `POST /api/v1/alerts`
- **AND** 客户端 SHALL refresh the alert table after a successful response
- **AND** 客户端 SHALL display the latest operation result from the API client

#### Scenario: 必填字段缺失时阻止创建

- **GIVEN** 操作员未填写告警编号、设备编号、级别或标题
- **WHEN** 操作员提交创建告警
- **THEN** 客户端 SHALL not treat the operation as successful
- **AND** 客户端 SHALL show a Chinese message describing the missing condition

### Requirement: Qt 客户端资产状态更新

Qt 客户端 SHALL provide an authenticated workflow for operators to update an equipment asset lifecycle status.

#### Scenario: 选中资产后更新状态

- **GIVEN** 操作员已通过 Qt 客户端登录后端
- **AND** 资产页面存在一条选中的资产
- **WHEN** 操作员选择新的资产状态并提交
- **THEN** 客户端 SHALL call `PATCH /api/v1/assets/{id}/status`
- **AND** 客户端 SHALL refresh the asset table after a successful response
- **AND** 客户端 SHALL display the latest operation result from the API client

#### Scenario: 未选择资产时阻止更新

- **GIVEN** 操作员未选择资产
- **WHEN** 操作员提交状态更新
- **THEN** 客户端 SHALL not treat the operation as successful
- **AND** 客户端 SHALL show a Chinese message describing the missing selection

### Requirement: Qt client HTTP login
The system SHALL allow the Qt client to authenticate through the backend HTTP login endpoint when the backend is reachable.

#### Scenario: User logs in from Qt client with backend available
- **WHEN** a user submits credentials in the Qt client and the configured backend accepts them
- **THEN** the client stores the returned session token for subsequent API requests
- **AND** the UI indicates that it is connected to the backend

### Requirement: Qt client asset list synchronization
The system SHALL allow the Qt client to load equipment assets from the backend HTTP asset endpoint after login.

#### Scenario: Asset list is loaded from backend
- **WHEN** the Qt client has a valid token and requests the asset list
- **THEN** it displays asset rows returned by the backend

### Requirement: Qt client offline fallback
The system SHALL keep offline demo data available when backend HTTP calls fail.

#### Scenario: Backend is unavailable
- **WHEN** the Qt client cannot reach the configured backend or receives an invalid response
- **THEN** it keeps local demo data visible and shows an offline fallback message

### Requirement: Qt client monitoring state synchronization
The system SHALL allow the Qt client to load runtime monitoring state rows from the backend after login.

#### Scenario: Monitoring states are loaded from backend
- **WHEN** the Qt client has a valid token and requests monitoring states
- **THEN** it displays rows returned by the backend monitoring endpoint

### Requirement: Qt client alert list synchronization
The system SHALL allow the Qt client to load alert rows from the backend after login.

#### Scenario: Alerts are loaded from backend
- **WHEN** the Qt client has a valid token and requests alerts
- **THEN** it displays rows returned by the backend alert endpoint

### Requirement: Qt client monitoring and alert fallback
The system SHALL keep local monitoring and alert demo rows available when backend calls fail.

#### Scenario: Backend list request fails
- **WHEN** the Qt client cannot synchronize monitoring states or alerts
- **THEN** it keeps local demo rows visible and shows an offline fallback mode for that page

### Requirement: Qt 客户端运行状态写入

Qt 客户端 SHALL provide an authenticated workflow for operators to submit runtime monitoring states to the backend.

#### Scenario: 写入运行状态后刷新列表

- **GIVEN** 操作员已通过 Qt 客户端登录后端
- **WHEN** 操作员填写设备编号、运行状态、严重度和指标摘要并提交
- **THEN** 客户端 SHALL call `POST /api/v1/monitoring/states`
- **AND** 客户端 SHALL refresh the runtime monitoring table after a successful response
- **AND** 客户端 SHALL display the latest operation result from the API client

#### Scenario: 未登录或字段不完整时阻止写入

- **GIVEN** 操作员未登录后端或未填写必需字段
- **WHEN** 操作员提交运行状态
- **THEN** 客户端 SHALL not treat the operation as successful
- **AND** 客户端 SHALL show a Chinese message describing the missing condition

### Requirement: 操作审计事件仓储

系统 SHALL 使用统一仓储保存关键业务操作审计事件，事件至少包含编号、操作者、动作、资源类型、资源编号、结果、追踪号和发生时间。

#### Scenario: 审计服务记录事件

- **WHEN** 后端服务提交一条缺少编号或发生时间的操作审计事件
- **THEN** 系统 SHALL 自动补齐审计编号和发生时间
- **AND** 事件 SHALL 写入当前配置的内存或 MySQL 仓储

### Requirement: 操作审计 HTTP 查询

系统 SHALL 提供受权限保护的操作审计查询接口。

#### Scenario: 管理员查询操作审计

- **GIVEN** 管理员已登录并持有 `audit:read` 权限
- **WHEN** 管理员调用 `GET /api/v1/audit/events`
- **THEN** 后端 SHALL 返回最近的操作审计事件数组
- **AND** 每条记录 SHALL 包含操作者、动作、资源、结果、追踪号和发生时间

#### Scenario: 非授权用户被拒绝

- **GIVEN** 操作员已登录但没有 `audit:read` 权限
- **WHEN** 操作员调用 `GET /api/v1/audit/events`
- **THEN** 后端 SHALL 返回 HTTP 403

### Requirement: 关键操作自动审计

系统 SHALL 对登录成功、告警通知派发和告警通知重试写入操作审计事件。

#### Scenario: 登录成功写入审计

- **WHEN** 用户使用有效凭据登录成功
- **THEN** 后端 SHALL 写入 `auth.login` 操作审计事件

#### Scenario: 告警通知派发写入审计

- **WHEN** 操作员派发待投递告警通知
- **THEN** 后端 SHALL 写入 `alert-notification.dispatch` 操作审计事件
- **AND** 审计资源编号 SHALL 包含成功、失败和跳过数量摘要

### Requirement: Qt 客户端操作审计页面

Qt 客户端 SHALL 在管理员登录后展示操作审计事件。

#### Scenario: 客户端同步操作审计

- **WHEN** Qt 客户端已登录并刷新在线数据
- **THEN** 客户端 SHALL 调用 `GET /api/v1/audit/events`
- **AND** “操作审计”页面 SHALL 展示编号、用户、动作、资源类型、资源编号、结果、追踪和时间

#### Scenario: 审计查询失败时展示离线兜底

- **WHEN** 后端不可用、未登录或审计查询失败
- **THEN** Qt 客户端 SHALL 保留本地演示审计记录
- **AND** 客户端 SHALL 使用中文状态消息说明当前为离线兜底数据

### Requirement: Qt client work order creation
The system SHALL allow the Qt client to create maintenance work orders through the backend after login.

#### Scenario: Operator creates a work order
- **WHEN** an authenticated operator submits work-order id, asset id, and summary
- **THEN** the client calls the backend work-order creation endpoint and refreshes the work-order list

### Requirement: Qt client work order assignment
The system SHALL allow the Qt client to assign a selected maintenance work order through the backend.

#### Scenario: Operator assigns a selected work order
- **WHEN** an authenticated operator selects a work order and submits an assignee
- **THEN** the client calls the backend work-order assignment endpoint and refreshes the work-order list

### Requirement: Qt client work order creation fallback
The system SHALL keep work-order creation and assignment non-blocking when backend calls fail.

#### Scenario: Backend creation or assignment request fails
- **WHEN** the Qt client cannot create or assign a work order
- **THEN** it keeps the visible work-order list available and shows a Chinese failure message

### Requirement: Qt 客户端从告警生成工单

Qt 客户端 SHALL provide an authenticated workflow for operators to create a maintenance work order from a selected alert.

#### Scenario: 选中告警后生成工单

- **GIVEN** 操作员已通过 Qt 客户端登录后端
- **AND** 告警中心存在一条选中的告警
- **WHEN** 操作员填写处置摘要并提交生成工单
- **THEN** 客户端 SHALL call `POST /api/v1/work-orders/from-alert` with the selected `alertId`
- **AND** 客户端 SHALL refresh the work order table after a successful response
- **AND** 客户端 SHALL display the latest operation result from the API client

#### Scenario: 未选择告警时阻止生成

- **GIVEN** 操作员未选择告警
- **WHEN** 操作员提交生成工单
- **THEN** 客户端 SHALL not treat the operation as successful
- **AND** 客户端 SHALL show a Chinese message describing the missing selection

### Requirement: Qt client work order synchronization
The system SHALL allow the Qt client to load maintenance work order rows from the backend after login.

#### Scenario: Work orders are loaded from backend
- **WHEN** the Qt client has a valid token and requests work orders
- **THEN** it displays rows returned by the backend work-order endpoint

### Requirement: Qt client work order transitions
The system SHALL allow the Qt client to submit basic work-order lifecycle transitions for a selected work order.

#### Scenario: User starts a selected work order
- **WHEN** an authenticated user selects a work order and starts processing
- **THEN** the client calls the backend start endpoint and refreshes the work-order list

#### Scenario: User completes a selected work order
- **WHEN** an authenticated user selects a work order and provides a completion result
- **THEN** the client calls the backend complete endpoint and refreshes the work-order list

### Requirement: Qt client work order fallback
The system SHALL keep local work-order demo rows available when backend calls fail.

#### Scenario: Backend work-order request fails
- **WHEN** the Qt client cannot synchronize or transition work orders
- **THEN** it keeps local demo rows visible and shows an offline fallback message

### Requirement: 操作审计导出权限

系统 SHALL 使用独立权限控制操作审计导出能力。

#### Scenario: 无导出权限用户被拒绝

- **GIVEN** 操作员已登录但没有 `audit:export` 权限
- **WHEN** 操作员调用 `GET /api/v1/audit/events/export`
- **THEN** 后端 SHALL 返回 HTTP 403

#### Scenario: 管理员具备默认导出权限

- **GIVEN** 默认管理员角色已经初始化
- **WHEN** 管理员登录后请求导出操作审计
- **THEN** 后端 SHALL allow the export request

### Requirement: 操作审计 CSV 导出接口

系统 SHALL 支持按当前筛选条件导出操作审计 CSV。

#### Scenario: 管理员导出筛选后的审计记录

- **GIVEN** 管理员已登录并持有 `audit:export` 权限
- **WHEN** 管理员调用 `GET /api/v1/audit/events/export?actor=admin&action=auth.login`
- **THEN** 后端 SHALL return HTTP 200 with `text/csv` content
- **AND** CSV SHALL contain the header `id,actor,action,resourceType,resourceId,result,traceId,occurredAt`
- **AND** CSV SHALL contain only records matching the supplied filters

#### Scenario: 导出操作自身被审计

- **WHEN** 管理员成功导出操作审计 CSV
- **THEN** 后端 SHALL record an `operation-audit.export` audit event
- **AND** the event resource id SHALL include the exported record count

### Requirement: Qt 客户端操作审计导出

Qt 客户端 SHALL allow administrators to export operation audit records from the current filter context.

#### Scenario: 客户端导出当前筛选审计

- **GIVEN** “操作审计”页面已经打开
- **WHEN** 管理员点击导出
- **THEN** Qt 客户端 SHALL call `GET /api/v1/audit/events/export` with the selected filters
- **AND** 客户端 SHALL save the returned CSV to a user-selected local path

#### Scenario: 导出失败时展示中文提示

- **WHEN** 后端拒绝导出或请求失败
- **THEN** Qt 客户端 SHALL show a Chinese failure message and SHALL NOT create an empty export file

### Requirement: CI 覆盖配置预检
系统 SHALL 在 GitHub Actions 中执行独立的配置预检质量门，确保部署配置、迁移脚本和 CMake presets 的仓库级约束不会只停留在本地验证。

#### Scenario: 推送配置变更

- **GIVEN** 提交修改了 compose、迁移脚本、CMake presets 或示例配置
- **WHEN** GitHub Actions 运行 CI
- **THEN** CI SHALL 执行 `deployment/preflight.ps1` 并在预检失败时阻断合入

#### Scenario: presets 语法或宏不可解析

- **GIVEN** 提交修改了 `CMakePresets.json`
- **WHEN** GitHub Actions 运行配置预检 job
- **THEN** CI SHALL 执行 `cmake --list-presets=configure` 并在 presets 无法解析时失败

### Requirement: 操作审计筛选查询

系统 SHALL 支持管理员按关键字段筛选操作审计事件。

#### Scenario: 按操作者和动作筛选

- **GIVEN** 管理员已登录并持有 `audit:read` 权限
- **WHEN** 管理员调用 `GET /api/v1/audit/events?actor=operator&action=alert-notification.dispatch`
- **THEN** 后端 SHALL 仅返回匹配操作者和动作的操作审计事件

#### Scenario: 按资源类型和结果筛选

- **GIVEN** 已存在告警通知派发审计事件
- **WHEN** 管理员调用 `GET /api/v1/audit/events?resourceType=alert-notification-batch&result=success`
- **THEN** 后端 SHALL 仅返回匹配资源类型和结果的审计事件

### Requirement: 操作审计分页查询

系统 SHALL 在审计查询传入分页参数时返回分页对象，并在未传分页参数时保持原数组响应兼容性。

#### Scenario: 分页查询返回总数和当前页

- **WHEN** 管理员调用 `GET /api/v1/audit/events?limit=20&offset=0`
- **THEN** 后端 SHALL 返回对象格式的 `data` 字段
- **AND** `data.items` SHALL contain the current page of operation audit events
- **AND** `data.total`, `data.limit`, and `data.offset` SHALL describe the full filtered result set and page request

#### Scenario: 无分页参数保持数组响应

- **WHEN** 管理员调用 `GET /api/v1/audit/events` 且未传入 `limit` 或 `offset`
- **THEN** 后端 SHALL 返回数组格式的 `data` 字段

#### Scenario: 无效分页参数被拒绝

- **WHEN** 管理员传入小于 1 或大于 100 的 `limit`，或传入负数 `offset`
- **THEN** 后端 SHALL return HTTP 400 with a Chinese validation message

### Requirement: Qt 客户端操作审计筛选分页控件

Qt 客户端 SHALL allow administrators to filter and page operation audit events from the desktop UI.

#### Scenario: 管理员筛选操作审计

- **GIVEN** “操作审计”页面已经打开
- **WHEN** 管理员填写用户、动作、资源类型或结果条件并点击刷新
- **THEN** Qt 客户端 SHALL request `GET /api/v1/audit/events` with the selected filters and pagination parameters
- **AND** 审计表格 SHALL display only the returned page

#### Scenario: 管理员翻页查看审计记录

- **GIVEN** 当前筛选条件存在多页审计记录
- **WHEN** 管理员点击上一页或下一页
- **THEN** Qt 客户端 SHALL update `offset` and refresh the audit table
- **AND** 页面提示 SHALL show the visible range and total count in Chinese

### Requirement: CI 后端基础作业可移植

The CI backend foundation job SHALL build and test the backend foundation without requiring a Visual Studio generator.

#### Scenario: GitHub Actions 运行后端基础测试

- **GIVEN** GitHub Actions receives a push or pull request
- **WHEN** the backend foundation job configures CMake
- **THEN** the job SHALL use a runner and CMake configuration that does not require a local Visual Studio installation
- **AND** the job SHALL build `induspilot-backend-tests`
- **AND** the job SHALL run CTest with failure output enabled

### Requirement: AI Provider 鉴权配置
系统 SHALL 支持为 HTTP AI Provider 配置鉴权头，并避免在状态信息中泄露密钥。

#### Scenario: 配置 API Key

- **GIVEN** `ai.api_key` 非空
- **WHEN** HTTP AI Provider 发起推理请求
- **THEN** 请求 SHALL 包含由 `ai.auth_header` 和 `ai.auth_scheme` 组成的鉴权头
- **AND** 服务状态 SHALL 仅说明已配置鉴权头，不得输出密钥明文

#### Scenario: 未配置 API Key

- **GIVEN** `ai.api_key` 为空
- **WHEN** HTTP AI Provider 发起推理请求
- **THEN** 请求 SHALL 不发送鉴权头

### Requirement: AI Provider 响应 schema 校验
系统 SHALL 校验 HTTP AI Provider 响应是否包含可用于业务展示和审计的文本内容。

#### Scenario: JSON 响应缺少文本字段

- **GIVEN** Provider 返回 2xx JSON 响应
- **AND** 响应不包含 `content`、`summary`、`text`、`output_text` 或兼容 OpenAI `choices/output` 的文本字段
- **WHEN** 后端处理 Provider 响应
- **THEN** AI 结果 SHALL 标记为 unavailable
- **AND** 系统 SHALL 使用本地规则降级描述继续业务流程

#### Scenario: 严格结构化响应模式收到非 JSON

- **GIVEN** `ai.require_structured_response` 为 true
- **WHEN** Provider 返回非 JSON 响应
- **THEN** AI 结果 SHALL 标记为 unavailable
- **AND** 系统 SHALL 记录降级原因

### Requirement: 本地依赖 compose 不提交真实密钥且默认仅本机暴露
系统 SHALL 保证提交到仓库的 compose 依赖配置不包含可直接复用的真实数据库/缓存密钥，并默认仅绑定到本机回环地址。

#### Scenario: 启动本地依赖前配置密钥

- **GIVEN** 开发者准备启动 `deployment/docker-compose.yml`
- **WHEN** `.env` 中缺少 MySQL、Redis 或 MongoDB 必需密钥
- **THEN** compose 配置 SHALL 拒绝使用空值或仓库内固定密码启动核心依赖

#### Scenario: 默认端口暴露范围

- **GIVEN** 开发者使用 `.env.example` 派生本地 `.env`
- **WHEN** 启动 MySQL、Redis 和 MongoDB
- **THEN** 这些服务 SHALL 默认绑定 `127.0.0.1`，除非操作者显式修改对应 `*_BIND` 变量

#### Scenario: 依赖健康状态可观测

- **GIVEN** compose 启动核心依赖容器
- **WHEN** 容器完成启动
- **THEN** MySQL、Redis 和 MongoDB SHALL 提供 healthcheck 以反映依赖可用性

### Requirement: 登录失败锁定与限流
系统 SHALL 在短时间连续登录失败达到阈值后临时锁定该登录主体，并提供可配置的失败窗口、失败阈值和锁定时长。

#### Scenario: 连续失败触发锁定

- **GIVEN** 登录锁定策略已启用
- **WHEN** 同一用户名在失败窗口内连续失败达到阈值
- **THEN** 后续登录 SHALL 返回 `AUTHENTICATION_LOCKED`
- **AND** HTTP 响应 SHALL 使用 `429 Too Many Requests`
- **AND** HTTP 响应 SHALL 包含 `Retry-After`

#### Scenario: 失败登录写入审计

- **GIVEN** 登录请求认证失败或命中锁定
- **WHEN** HTTP 服务处理该请求
- **THEN** 系统 SHALL 写入 `auth.login.failed` 或 `auth.login.locked` 操作审计事件
- **AND** 审计事件 SHALL 包含当前请求追踪编号

#### Scenario: 成功登录清除失败状态

- **GIVEN** 用户名存在历史失败登录记录但尚未锁定
- **WHEN** 用户使用正确凭据登录成功
- **THEN** 系统 SHALL 清除该用户名的失败登录状态

### Requirement: CMake presets 不依赖固定用户目录
系统 SHALL 保证提交到仓库的 CMake presets 不写死具体 Windows 用户目录中的 vcpkg 路径，并通过 `VCPKG_ROOT` 定位 vcpkg toolchain 与测试运行时库路径。

#### Scenario: 在不同 Windows 用户目录构建 Redis preset

- **GIVEN** 开发机已设置 `VCPKG_ROOT` 且安装了项目所需 vcpkg 依赖
- **WHEN** 执行 `cmake --preset dev-redis`
- **THEN** CMake SHALL 从 `VCPKG_ROOT` 解析 vcpkg toolchain，而不是依赖 `C:/Users/20106/vcpkg`

#### Scenario: 预检扫描 CMakePresets

- **GIVEN** `CMakePresets.json` 出现 `C:/Users/<name>/vcpkg` 或 `C:\Users\<name>\vcpkg`
- **WHEN** 运行部署预检
- **THEN** 预检 SHALL 失败并提示移除本机 vcpkg 用户路径硬编码

### Requirement: MySQL 增量迁移可重复执行
系统 SHALL 保证已纳入部署预检的 MySQL 增量迁移在目标列或索引已存在时不会因重复创建而失败。

#### Scenario: 重复执行审计完整性迁移

- **GIVEN** `operation_audit_events` 已存在 `previous_hash`、`event_hash` 和 `idx_operation_audit_event_hash`
- **WHEN** 再次执行 `009_operation_audit_integrity_schema.sql`
- **THEN** 迁移 SHALL 跳过已有列和索引，并继续登记 `009_operation_audit_integrity_schema`

#### Scenario: 预检发现非幂等列或索引变更

- **GIVEN** MySQL 迁移脚本通过 `ALTER TABLE` 添加列或索引
- **WHEN** 脚本没有使用 `INFORMATION_SCHEMA` 或 `IF NOT EXISTS` 判断目标对象是否已存在
- **THEN** 部署预检 SHALL 报告失败并指出对应脚本

### Requirement: Runtime repository selection
The system SHALL support explicit runtime repository selection for services that have multiple repository implementations.

#### Scenario: Default runtime uses memory repositories
- **WHEN** no repository store is configured
- **THEN** the system uses memory repositories so local development and integration tests remain self-contained

#### Scenario: Runtime selects MySQL repositories
- **WHEN** the repository store is configured as `mysql`
- **THEN** services with MySQL repository implementations use configured MySQL connectivity instead of memory repositories

### Requirement: Qt 工业运维工作台摘要
Qt 客户端 SHALL 在总览页展示跨模块运营摘要，帮助用户快速判断告警到工单闭环状态。

#### Scenario: 登录后查看总览

- **GIVEN** 用户已登录或处于离线演示模式
- **WHEN** 用户打开 Qt 客户端总览页
- **THEN** 总览页 SHALL 显示资产数量、关键运行状态数量、待处理告警数量、活跃工单数量、AI 记录数量和审计事件数量
- **AND** 总览页 SHALL 显示当前数据来源为后端同步或离线演示数据

#### Scenario: 业务数据刷新后更新摘要

- **GIVEN** 用户在资产、运行监控、告警或工单页面执行操作
- **WHEN** 客户端刷新对应表格
- **THEN** 总览页摘要 SHALL 使用最新客户端数据更新

#### Scenario: 闭环下一步提示

- **GIVEN** 总览页已加载当前数据
- **WHEN** 存在关键运行状态、待处理告警或活跃工单
- **THEN** 客户端 SHALL 展示面向告警到工单闭环的下一步提示

### Requirement: 审计哈希链追加使用真实最新事件
系统 SHALL 在记录新的操作审计事件时使用仓储提供的真实最新事件哈希作为 `previousHash`，而不是依赖用于 UI 查询的分页或倒序列表。

#### Scenario: 记录新审计事件

- **GIVEN** 仓储中已经存在一条或多条审计事件
- **WHEN** 系统记录新的审计事件
- **THEN** 新事件的 `previousHash` SHALL 等于仓储 `latest()` 返回事件的 `eventHash`

#### Scenario: 同进程并发记录审计事件

- **GIVEN** 多个请求在同一后端进程中同时记录审计事件
- **WHEN** 服务层追加哈希链
- **THEN** 系统 SHALL 串行化当前进程内的 `latest()` 与 `save()` 操作，降低同一上一哈希被重复使用的风险

### Requirement: 审计完整性校验覆盖全量写入顺序
系统 SHALL 使用按写入顺序排列的全量审计事件执行哈希链完整性校验，并保留查询接口最近优先的展示语义。

#### Scenario: 校验超过查询窗口的审计链

- **GIVEN** MySQL 审计表中的事件数量超过查询列表窗口
- **WHEN** 调用完整性校验
- **THEN** 系统 SHALL 使用 `listForIntegrity()` 全量正序事件，而不是 `list()` 最近 500 条倒序事件

#### Scenario: 查询审计事件

- **GIVEN** 用户查询操作审计事件
- **WHEN** 系统返回查询结果
- **THEN** 查询接口 SHALL 继续返回最近事件优先的列表

### Requirement: 开发文档描述当前审计完整性能力

开发文档 SHALL 描述当前操作审计查询、CSV 导出和哈希链完整性校验接口，且不得把已实现的依赖探测描述为旧版临时实现。

#### Scenario: 开发者查看后端 HTTP 文档

- **GIVEN** 开发者打开后端 HTTP 服务文档
- **WHEN** 开发者查看健康检查与操作审计章节
- **THEN** 文档 SHALL describe dependency connectivity probing
- **AND** 文档 SHALL include `GET /api/v1/audit/integrity`
- **AND** 文档 SHALL describe audit hash-chain fields and verification result

### Requirement: 示例配置说明运行时优先级

示例配置 SHALL 明确 MySQL `uri` 非空时优先于 host/port/database/user/password 组合连接参数。

#### Scenario: 开发者配置 MySQL URI

- **GIVEN** 开发者打开 `config/backend.example.yaml`
- **WHEN** 开发者查看 `mysql.uri` 注释
- **THEN** 注释 SHALL explain that non-empty `uri` is used first by the MySQL repository

### Requirement: Database schema version baseline
The system SHALL provide a repeatable schema version baseline for MySQL initialization scripts.

#### Scenario: MySQL initialization scripts are applied
- **WHEN** the MySQL initialization scripts are executed in order
- **THEN** the database records the applied schema versions in a dedicated migration tracking table

### Requirement: Deployment preflight checks
The system SHALL provide a local deployment preflight check for runtime dependency readiness.

#### Scenario: Operator runs preflight before starting production-like backend
- **WHEN** an operator runs the preflight script from the repository root
- **THEN** the script checks required configuration, database scripts, schema version declarations, and compose dependency definitions
- **AND** it returns a non-zero exit code when required checks fail
#### Scenario: Runtime selects MySQL repositories for operational modules
- **WHEN** the repository store is configured as `mysql`
- **THEN** identity, asset, alert, work-order, runtime-state, and AI-interaction services use configured MySQL repositories where implementations exist

### Requirement: CI 质量门禁
系统 SHALL 在 CI 中执行独立的质量门禁，验证工程格式化配置、静态分析配置、CMake presets 和工作流基础约束。

#### Scenario: CI 运行质量门禁
- **GIVEN** GitHub Actions 收到 push 或 pull request
- **WHEN** 执行质量门禁 job
- **THEN** CI SHALL 校验 `.clang-format`、`.clang-tidy`、`CMakePresets.json` 和工作流结构
- **AND** CI SHALL 在缺少 clang 工具链或基础约束失败时阻断合入

#### Scenario: 开发者本地运行质量门禁
- **GIVEN** 开发者在仓库根目录运行质量门禁脚本
- **WHEN** 本地未安装 clang 工具链
- **THEN** 脚本 SHALL 完成仓库结构和配置约束检查，并提示本地跳过 clang 工具版本检查

### Requirement: Agent 工作流展示证据
系统 SHALL 提供可生成的 Agent 工作流证据包，用于展示需求、规格、实现、验证、提交和 CI 的交付闭环。

#### Scenario: 生成 Agent 证据包
- **GIVEN** 开发者位于仓库工作区
- **WHEN** 运行 Agent 工作流证据脚本
- **THEN** 脚本 SHALL 生成 JSON 结构化证据和 Markdown 展示摘要
- **AND** 证据 SHALL 包含最近提交、OpenSpec 归档、CI job 和工业闭环摘要的可用信息

#### Scenario: 查看 Agent 交付说明
- **GIVEN** 访问项目文档
- **WHEN** 开发者打开 Agent 工作流展示文档
- **THEN** 文档 SHALL 描述从 OpenSpec 到 GitHub Actions 的闭环链路
- **AND** 文档 SHALL 说明工业 AI 诊断中的 agent 编排边界

### Requirement: Prometheus 可观测性指标
系统 SHALL 在 Drogon HTTP 运行时提供 Prometheus 文本格式指标端点，用于观测请求、错误、AI 调用和关键运维闭环动作。

#### Scenario: 运维系统抓取指标
- **GIVEN** Drogon 后端正在运行
- **WHEN** 运维系统调用 `GET /metrics`
- **THEN** 后端 SHALL 返回 `text/plain` Prometheus 指标文本
- **AND** 指标 SHALL 包含 HTTP 请求总数、错误总数、AI 请求总数、告警关闭次数和工单关闭次数

#### Scenario: HTTP 路径指标避免高基数
- **GIVEN** 请求路径包含资产、告警、通知或工单编号
- **WHEN** 后端记录 HTTP 路由指标
- **THEN** 指标标签 SHALL 使用归一化路径，不得直接暴露具体业务对象编号

#### Scenario: HTTP smoke 校验 metrics
- **GIVEN** HTTP 集成冒烟测试运行
- **WHEN** 测试访问 `/metrics`
- **THEN** 测试 SHALL 验证 Prometheus 指标文本包含请求总数、错误总数、AI 请求和归一化路由

