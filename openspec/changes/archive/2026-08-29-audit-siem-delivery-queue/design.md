# Design: 审计 SIEM 耐久投递队列

## Architecture

`AuditService::record` 仍先在互斥区内生成并保存审计哈希链事件。保存成功后，若配置了 SIEM sink，则向 `AuditDeliveryQueueRepository` 写入以 audit event code 为唯一键的任务并唤醒 worker。队列写入失败被隔离，不影响审计保存。

`AuditDeliveryWorker` 由 `AuditService` 拥有。它使用持久化租约领取到期任务，调用现有受限 SIEM HTTP sender；成功任务标记为 `sent`，失败任务按指数退避标记 `retrying`，达到最大尝试次数后标记 `dead_letter`。析构时 worker 被通知退出并 join。

## Storage

新增 `audit_siem_deliveries`：任务与 `operation_audit_events.event_code` 一一对应，保存状态、尝试次数、错误摘要、下一次尝试时间、租约和最大次数。MySQL 领取采用先更新租约、后按 token 读取的模式，与告警通知队列保持一致。内存仓储实现相同接口以支持开发和单元测试。

## Observability

指标保留固定 `outcome` 集合 `sent`、`retrying`、`dead_letter`，通道固定为 `siem`。队列快照暴露 `queued`、`retrying` 和 `dead_letter` 的当前数量，避免将事件或错误信息作为标签。

## Failure Handling

- URL、allowlist、DNS 或 HTTP 状态异常视为一次可重试投递失败。
- 队列仓储异常和 sink 异常被 worker 捕获并限制错误长度。
- 审计记录保存与队列投递互不构成事务性失败条件；审计哈希链始终优先。
