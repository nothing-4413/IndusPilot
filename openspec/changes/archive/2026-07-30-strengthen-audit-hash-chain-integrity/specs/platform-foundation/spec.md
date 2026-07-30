## ADDED Requirements

### Requirement: 审计哈希链追加使用真实最新事件
系统 SHALL 在记录新的操作审计事件时使用仓储提供的真实最新事件哈希作为 `previousHash`，而不是依赖用于 UI 查询的分页或倒序列表。

#### Scenario: 记录新审计事件

- **GIVEN** 仓储中已经存在一条或多条审计事件
- **WHEN** 系统记录新的审计事件
- **THEN** 新事件的 `previousHash` SHALL 等于仓储 `latest()` 返回事件的 `eventHash`

#### Scenario: 同进程并发记录审计事件

- **GIVEN** 多个请求在同一后端进程中同时记录审计事件
- **WHEN** 服务层追加哈希链
- **THEN** 系统 SHALL 串行化当前进程内的 `latest()` 与 `save()` 操作，降低同一上一哈希被重复使用的风险

### Requirement: 审计完整性校验覆盖全量写入顺序
系统 SHALL 使用按写入顺序排列的全量审计事件执行哈希链完整性校验，并保留查询接口最近优先的展示语义。

#### Scenario: 校验超过查询窗口的审计链

- **GIVEN** MySQL 审计表中的事件数量超过查询列表窗口
- **WHEN** 调用完整性校验
- **THEN** 系统 SHALL 使用 `listForIntegrity()` 全量正序事件，而不是 `list()` 最近 500 条倒序事件

#### Scenario: 查询审计事件

- **GIVEN** 用户查询操作审计事件
- **WHEN** 系统返回查询结果
- **THEN** 查询接口 SHALL 继续返回最近事件优先的列表