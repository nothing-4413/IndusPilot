# Change: 审计 SIEM 耐久投递队列

## ADDED Requirements

### Requirement: 审计 SIEM 投递必须具有耐久重试状态
系统 SHALL 在启用 SIEM webhook 时为已持久化的操作审计事件创建可恢复的投递任务；投递失败不得阻止审计事件和哈希链保存。

#### Scenario: SIEM 短暂不可用
- **GIVEN** SIEM webhook 已启用且目标暂时不可用
- **WHEN** 系统保存新的操作审计事件
- **THEN** 审计事件 SHALL 被保存
- **AND** 对应 SIEM 任务 SHALL 进入 `retrying` 并安排后续尝试

#### Scenario: 服务重启后恢复任务
- **GIVEN** MySQL 中存在未完成的 SIEM 投递任务
- **WHEN** 后端启动并启用 SIEM webhook
- **THEN** worker SHALL 领取到期任务并继续投递

#### Scenario: 重试耗尽
- **GIVEN** SIEM 任务连续失败并达到配置的最大尝试次数
- **WHEN** worker 完成最后一次尝试
- **THEN** 任务 SHALL 被标记为 `dead_letter`
- **AND** 后续自动 worker 不得再次领取该任务

### Requirement: SIEM 投递 worker 必须受控停止并可观测
系统 SHALL 在 SIEM worker 停止时等待其结束，并公开不包含敏感数据的投递结果和队列状态指标。

#### Scenario: 进程关闭
- **GIVEN** SIEM worker 正在等待或轮询任务
- **WHEN** 包含 AuditService 的运行时被销毁
- **THEN** worker SHALL 被唤醒、停止并在析构返回前结束

#### Scenario: 运维采集指标
- **WHEN** Prometheus 拉取指标
- **THEN** 输出 SHALL 包含 SIEM 投递的 `sent`、`retrying` 和 `dead_letter` 计数
- **AND** 输出 SHALL 包含待投递、重试中和死信任务的当前数量
