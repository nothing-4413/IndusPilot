# platform-foundation Specification

## ADDED Requirements

### Requirement: 启动配置错误与依赖不可用分离
系统 SHALL 在启动时校验静态配置；静态配置错误 SHALL 阻止 HTTP runtime 启动并返回失败状态。外部依赖暂时不可用 SHALL NOT 将进程标记为已停止，而 SHALL 由 readiness 反映。

#### Scenario: 静态配置无效
- **WHEN** server 端口、仓储类型、会话存储类型或启用的 provider 配置非法
- **THEN** `Application::start()` SHALL 失败
- **AND** HTTP listener SHALL NOT 启动

#### Scenario: 依赖暂时不可用
- **WHEN** 必需 MySQL 或 Redis 在启动时不可达
- **THEN** 进程 SHALL 保持运行并提供 liveness
- **AND** readiness SHALL 返回 HTTP 503

### Requirement: 依赖 required/optional 语义
系统 SHALL 根据运行时配置计算依赖是否 required，并 SHALL 不探测 memory 模式未使用的外部依赖。

#### Scenario: memory 运行时
- **WHEN** repository_store 和 session_store 均为 memory
- **THEN** MySQL 和 Redis SHALL 不执行外部探测
- **AND** readiness SHALL 不因它们的状态失败

#### Scenario: MySQL/Redis required
- **WHEN** repository_store=mysql 或 session_store=redis
- **THEN** 对应依赖 SHALL 标记为 required
- **AND** 依赖不可用时 readiness SHALL 返回 HTTP 503

#### Scenario: AI 默认降级
- **WHEN** AI HTTP provider 启用但 `ai.required` 未开启
- **THEN** AI 不可用 SHALL NOT 阻断核心 readiness

### Requirement: 分离 liveness、readiness 和 startup 检查
系统 SHALL 提供独立的进程存活、依赖就绪和启动状态端点。

#### Scenario: 进程存活
- **WHEN** 调用 `GET /health/live`
- **THEN** endpoint SHALL 返回 HTTP 200
- **AND** SHALL NOT 执行外部依赖探测

#### Scenario: 依赖未就绪
- **WHEN** 调用 `GET /health/ready` 且任一 required 依赖不可用
- **THEN** endpoint SHALL 返回 HTTP 503
- **AND** 响应 SHALL 包含 required、available 和失败原因

#### Scenario: 启动完成
- **WHEN** 静态配置有效且 Application 已初始化
- **THEN** `GET /health/startup` SHALL 返回 HTTP 200
- **AND** 响应 SHALL 包含 startup 状态

### Requirement: readiness 支持依赖恢复
系统 SHALL 在探测缓存过期后重新评估依赖状态。

#### Scenario: 依赖恢复
- **GIVEN** readiness 曾因 required 依赖不可用返回 503
- **WHEN** 依赖恢复且下一次探测完成
- **THEN** readiness SHALL 返回 HTTP 200

### Requirement: 兼容旧 health 接口
系统 SHALL 保持 `GET /health` 的既有路径、HTTP 200 兼容语义和基础 JSON 字段，同时允许新增诊断字段。

#### Scenario: 旧客户端访问 health
- **WHEN** 客户端调用 `GET /health`
- **THEN** 响应 SHALL 继续包含 service、dependencies 和 warnings
- **AND** 新的 readiness 失败 SHALL NOT 改变该兼容接口的 HTTP 状态码
