# Change: 清理历史登录审计 token

## Why

旧版本后端曾将登录成功后的 session token 写入 `operation_audit_events.resource_id`。新代码已停止产生该问题，但已有 MySQL 数据仍可能保留可复用凭据，部署升级后仍可通过审计查询或导出暴露。

## What Changes

- 增加幂等 MySQL 迁移，将历史 `auth.login` session 事件改为用户资源标识。
- 将迁移加入 schema baseline、部署预检和真实 MySQL CRUD smoke。
- 更新运维文档，明确升级迁移会清理旧登录审计中的 token 字段。

## Non-Goals

- 本次不删除登录审计事件，不改变审计哈希链字段。
- 本次不迁移其他类型资源或猜测非登录事件中的敏感字段。
