# 设计：历史登录审计 token 清理

迁移只匹配 `action = 'auth.login'` 且 `resource_type = 'session'` 的旧事件，将 `resource_type` 改为 `user`，将 `resource_id` 改为同一事件的 `actor`。新版本写入的 `auth.login` 已使用 `user`，因此迁移可重复执行且不会修改新数据。

```sql
UPDATE operation_audit_events
SET resource_type = 'user', resource_id = actor
WHERE action = 'auth.login' AND resource_type = 'session';
```

迁移不触碰 `previous_hash` 或 `event_hash`，因为这些字段应保留原始审计记录的完整性链路；历史数据清理属于资源标识脱敏，不是事件删除。
