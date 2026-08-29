# Change: 审计 SIEM 耐久投递队列

## Why

现有 SIEM webhook 在审计记录完成后以 detached thread 进行一次尽力而为的请求。进程退出、瞬时网络失败和非成功 HTTP 响应都会丢失投递机会，且运维人员无法从指标中分辨重试或死信状态。

## What Changes

- 将 SIEM 投递任务写入独立的持久化队列，MySQL 模式下重启后可恢复。
- 引入带租约的后台 worker、指数退避重试和死信终态；投递异常绝不回滚审计事件。
- 让 worker 在服务析构时停止并等待，避免 detached thread 逃逸。
- 为 SIEM 投递结果和积压状态增加受限标签的 Prometheus 指标。

## Non-goals

- 不新增 SIEM 管理、重放或人工死信处理 API。
- 不改变审计哈希链、常规查询窗口或归档行为。
- 不改变既有告警通知队列。
