# platform-foundation Specification

## ADDED Requirements

### Requirement: 配置加载错误必须阻止启动

系统 SHALL 对配置文件读取、层级、section、key、整数和布尔值进行严格解析。任何加载错误 SHALL 进入 startup 错误集合，并使 `Application::start()` 失败且不启动 HTTP listener。

#### Scenario: 配置文件不存在
- **WHEN** 启动参数指向不存在的配置文件
- **THEN** startup SHALL 报告配置读取错误
- **AND** HTTP runtime SHALL 以配置错误退出

#### Scenario: 类型值非法
- **WHEN** 整数或布尔配置包含无法完整解析的值
- **THEN** 系统 SHALL 报告带字段名的解析错误
- **AND** SHALL NOT 静默使用默认值

#### Scenario: 未知配置字段
- **WHEN** 配置文件包含未知 section、key 或错误层级
- **THEN** 系统 SHALL 报告配置结构错误
- **AND** SHALL 阻止启动

### Requirement: readiness 探测不得持有应用状态锁

系统 SHALL 在执行外部依赖探测时释放应用状态锁。并发 readiness 请求 SHALL 共享同一轮进行中的探测结果，不得为同一缓存窗口重复创建探测轮次。

#### Scenario: 慢依赖探测期间访问 liveness
- **WHEN** readiness 正在执行外部探测
- **THEN** liveness 查询 SHALL 不等待该网络探测完成
- **AND** SHALL 返回当前进程状态

#### Scenario: 并发 readiness 请求
- **WHEN** 多个请求同时遇到过期缓存
- **THEN** SHALL 只有一个探测 owner 执行网络探测
- **AND** 其他请求 SHALL 等待并复用已发布结果

### Requirement: 依赖探测使用并发执行和统一 deadline

系统 SHALL 并行执行实际参与运行时的 MySQL、Redis 和 HTTP AI 探测，并 SHALL 使用包含 DNS 解析和连接阶段的统一 absolute deadline。

#### Scenario: 多依赖不可用
- **WHEN** MySQL、Redis 和 AI 同时启用且均不可用
- **THEN** 一轮探测的总耗时 SHALL 受单次 probe timeout 限制
- **AND** 每个依赖 SHALL 返回独立的 unavailable reason

#### Scenario: memory 模式
- **WHEN** 外部仓储和会话均为 memory 且 AI disabled
- **THEN** SHALL 不创建外部依赖探测任务
- **AND** readiness SHALL 继续立即可用

### Requirement: readiness 提供探测观测元数据

系统 SHALL 返回最近一轮探测的进行中状态、次数、耗时、失败次数、恢复次数和最近探测时间。失败/恢复计数 SHALL 只统计依赖状态边沿，不得把每个 readiness 请求重复计为失败。

#### Scenario: 依赖恢复
- **GIVEN** required 依赖曾经不可用
- **WHEN** 下一轮探测发现该依赖可用
- **THEN** readiness SHALL 恢复为 ready
- **AND** recoveryCount SHALL 增加

### Requirement: 兼容既有健康接口

系统 SHALL 保持 `/health` 的现有字段和 HTTP 200 兼容语义。新 metadata 只出现在 readiness 响应中，业务路由不得依赖探测内部实现。
