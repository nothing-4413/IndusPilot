## ADDED Requirements

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