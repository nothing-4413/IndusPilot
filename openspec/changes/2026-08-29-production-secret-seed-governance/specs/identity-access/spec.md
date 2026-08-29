# Change: 生产密钥与种子账号启动治理

## ADDED Requirements

### Requirement: 生产模式拒绝种子凭据兼容
系统 SHALL 支持显式生产安全模式，并在该模式下拒绝允许未轮换种子账号登录的配置。

#### Scenario: 生产模式与种子兼容开关冲突
- **GIVEN** `security.production_mode=true`
- **AND** `security.allow_seed_credentials=true`
- **WHEN** 应用启动
- **THEN** 静态配置校验 SHALL 失败
- **AND** HTTP listener SHALL NOT 启动

### Requirement: 生产 MySQL 运行时拒绝示例密钥
系统 SHALL 在生产模式使用 MySQL 仓储时拒绝空值、`change-me*` 和已发布演示 MySQL 密钥。

#### Scenario: 示例数据库密码
- **GIVEN** `security.production_mode=true` 且 `repository_store=mysql`
- **WHEN** MySQL password 为空或使用示例值
- **THEN** 配置校验 SHALL 失败

### Requirement: 生产部署预检验证秘密与治理开关
部署预检 SHALL 提供显式生产检查，且不得输出秘密值。

#### Scenario: 检查生产环境文件
- **WHEN** 操作员运行 `preflight.ps1 -RequireProductionSecrets`
- **THEN** 脚本 SHALL 验证 MySQL、Redis、MongoDB 的 Compose 密钥非空且非示例
- **AND** SHALL 要求生产模式启用且种子凭据兼容禁用
