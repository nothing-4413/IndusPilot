## ADDED Requirements

### Requirement: 历史登录审计 token 必须可幂等清理
系统 SHALL 提供可重复执行的 MySQL 迁移，将旧版本登录成功审计中的 session token 替换为用户标识，不得删除审计事件或破坏其哈希字段。

#### Scenario: 升级已有审计数据

- **GIVEN** `auth.login` 事件使用 `resource_type=session` 且 `resource_id` 保存旧 session token
- **WHEN** 执行历史审计清理迁移
- **THEN** 事件 SHALL 使用 `resource_type=user` 和稳定用户标识
- **AND** `previous_hash` 与 `event_hash` SHALL 保持不变

#### Scenario: 重复执行迁移

- **GIVEN** 历史审计清理迁移已经执行
- **WHEN** 再次执行同一迁移
- **THEN** 迁移 SHALL 成功完成
- **AND** 不得重复改变已清理事件或新增重复 schema 版本
