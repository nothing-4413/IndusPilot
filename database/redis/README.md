# Redis 基线

Redis 用于保存短生命周期和高频访问数据：

- `session:{token}`：用户会话
- `cache:{module}:{key}`：模块缓存
- `runtime:asset:{assetId}`：设备运行态快照
- `queue:alerts`：告警处理队列
- `rate-limit:{identity}`：限流计数

当前阶段仅定义职责和键空间，真实客户端接入将在后续数据访问层实现。