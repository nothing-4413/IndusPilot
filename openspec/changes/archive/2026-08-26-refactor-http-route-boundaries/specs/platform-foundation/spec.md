## ADDED Requirements

### Requirement: HTTP 路由按业务上下文注册
系统 SHALL 将 HTTP 路由注册拆分为独立的业务上下文 registrar，并通过统一的服务上下文接入应用服务。

#### Scenario: 新模块接入 HTTP

- **GIVEN** 新模块已经提供服务和仓储接口
- **WHEN** 新模块接入 HTTP runtime
- **THEN** 新模块 SHALL 通过独立的 `register<Context>Routes` 入口注册路由
- **AND** 新模块 SHALL 不需要把 handler 实现追加到现有中央路由函数

#### Scenario: 现有 API 保持兼容

- **GIVEN** HTTP 路由完成模块化拆分
- **WHEN** 客户端调用现有 API
- **THEN** API 路径和 HTTP method SHALL 保持不变
- **AND** 认证、权限、响应 envelope 和错误状态 SHALL 保持兼容

### Requirement: HTTP 公共能力与业务路由分离
系统 SHALL 将认证授权、错误响应、trace、metrics 和序列化等公共 HTTP 能力与业务路由分离。

#### Scenario: 业务路由使用公共守卫

- **WHEN** 业务 handler 处理受保护请求
- **THEN** handler SHALL 使用共享 session/permission guard
- **AND** handler SHALL 返回统一错误响应

### Requirement: HTTP 组合根集中组装依赖
系统 SHALL 在 composition root 中选择 memory/MySQL repository、memory/Redis session 和 AI provider，并向 route registrar 提供已组装服务。

#### Scenario: 新模块声明依赖

- **GIVEN** 新模块需要一个或多个既有服务
- **WHEN** 新模块注册路由
- **THEN** 依赖 SHALL 通过显式的 HTTP 服务上下文提供
- **AND** 路由文件 SHALL 不自行创建 repository 或 session store
