# platform-foundation Specification

## ADDED Requirements

### Requirement: Application 生命周期必须区分运行、排空和停止

系统 SHALL 暴露可查询的 running、draining 和 stopped 生命周期语义。开始排空和最终停止 SHALL 幂等。

#### Scenario: 收到停机请求
- **WHEN** runtime 收到 SIGTERM、SIGINT 或等价 shutdown 请求
- **THEN** Application SHALL 进入 draining 状态
- **AND** 后续 readiness SHALL 返回未就绪
- **AND** 在最终停止前 liveness SHALL 仍表示进程存活

#### Scenario: 重复停机请求
- **WHEN** 多次调用 shutdown
- **THEN** 系统 SHALL 只执行一次排空/停止协调
- **AND** SHALL NOT 重置或丢失已记录的生命周期状态

### Requirement: HTTP runtime 必须拒绝排空开始后的新业务请求

系统 SHALL 在所有业务 handler 前提供统一请求 gate。进入 draining 后新业务请求 SHALL 返回 HTTP `503`，已有请求 SHALL 允许完成。

#### Scenario: 新请求进入 draining 实例
- **WHEN** 请求在 draining 状态下通过 HTTP runtime
- **THEN** 非健康路径 SHALL 返回 `503`
- **AND** 响应 SHALL 使用稳定的 `SERVER_DRAINING` 错误码

#### Scenario: 已接受请求完成
- **GIVEN** 请求已经通过 gate 并开始执行
- **WHEN** Application 进入 draining
- **THEN** 该请求 SHALL 继续执行到 handler 完成
- **AND** shutdown coordinator SHALL 能够等待该请求计数归零

### Requirement: 健康检查必须反映排空状态

系统 SHALL 在 draining 期间允许健康端点返回状态；`/health/ready` SHALL 返回 `503`，`/health/live` SHALL 在进程尚未最终停止时返回 `200`。

#### Scenario: draining 期间健康检查
- **WHEN** 编排器轮询 `/health/live` 和 `/health/ready`
- **THEN** live SHALL 返回 `200`
- **AND** ready SHALL 返回 `503`

### Requirement: 信号和 listener 失败必须有明确 runtime 契约

系统 SHALL 将 SIGTERM/SIGINT 绑定到同一 shutdown coordinator，并 SHALL 将 listener 绑定失败映射为明确的非零退出码和诊断 stderr。

#### Scenario: 端口已被占用
- **WHEN** HTTP listener 无法绑定配置的 host/port
- **THEN** runtime SHALL 返回非零退出码
- **AND** stderr SHALL 包含 listener 地址
