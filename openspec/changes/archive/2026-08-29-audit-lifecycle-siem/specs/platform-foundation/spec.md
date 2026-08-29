# Change: 操作审计生命周期与 SIEM 投递

## ADDED Requirements

### Requirement: 审计保留窗口不得破坏完整性链
系统 SHALL 支持限制常规操作审计查询的可见时间窗口；该配置不得删除事件或改变完整性校验的输入。

#### Scenario: 配置常规查询窗口
- **GIVEN** `audit.retention_days` 为正整数
- **WHEN** 用户查询未指定更晚下限的操作审计
- **THEN** 返回内容 SHALL 不早于当前窗口下限
- **AND** 完整性报告 SHALL 继续验证全部历史记录

### Requirement: 管理员可生成完整审计归档
系统 SHALL 提供由 `audit:export` 保护的完整 CSV 归档接口，并对归档行为写入审计链。

#### Scenario: 管理员归档历史记录
- **WHEN** 具备 `audit:export` 的用户请求审计归档
- **THEN** 返回的 CSV SHALL 按筛选条件覆盖完整历史链而非仅保留窗口
- **AND** 系统 SHALL 记录 `operation-audit.archive` 事件

### Requirement: SIEM webhook 投递是受限的尽力而为操作
系统 SHALL 支持默认关闭的外部 SIEM webhook，并且投递失败不得阻止操作审计记录持久化。

#### Scenario: 受限安全投递
- **GIVEN** SIEM webhook 已启用
- **WHEN** 写入新的操作审计事件
- **THEN** 目标 URL SHALL 为 HTTP(S)、主机 SHALL 在允许名单中、所有 DNS 地址 SHALL 为公网地址
- **AND** 连接 SHALL 使用已验证地址并保留 Host authority

#### Scenario: SIEM 不可用
- **GIVEN** SIEM webhook 请求失败或返回非成功状态
- **WHEN** 系统记录操作审计事件
- **THEN** 事件 SHALL 仍被保存并保持完整性链连续
