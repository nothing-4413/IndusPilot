## ADDED Requirements

### Requirement: 过期内存会话会被回收
系统 SHALL 在读取到过期或无效的内存会话时移除该存储条目。非正 session TTL SHALL 删除内存会话，保持与 Redis 会话存储一致。

#### Scenario: 零 TTL 会话不被保留

- **GIVEN** 内存会话存储收到 TTL 为零的会话保存请求
- **WHEN** 保存完成
- **THEN** 会话 SHALL 不可读取
- **AND** 存储中 SHALL 不保留该过期条目
