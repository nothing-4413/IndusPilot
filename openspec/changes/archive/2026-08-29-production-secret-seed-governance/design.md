# Design: 显式生产安全模式

`security.production_mode` 默认关闭，确保内存仓储与样例账户继续服务本地演示。启用后，静态配置校验在 listener 启动前拒绝 `allow_seed_credentials=true`。当 `repository_store=mysql` 时，还要求独立的 MySQL 密码字段存在且不是 `change-me*` 或已发布演示密码，避免 URI 覆盖掩盖不安全的配置。

`deployment/preflight.ps1 -RequireProductionSecrets` 读取未提交的部署 `.env`，验证 MySQL root/app、Redis、MongoDB root 密钥均非空且非示例，并要求 `INDUSPILOT_SECURITY_PRODUCTION_MODE=true` 与 `INDUSPILOT_SECURITY_ALLOW_SEED_CREDENTIALS=false`。它不会回显任何值。

现有种子账号的 `requires_password_rotation` 标记及 `rotate_seed_credentials.ps1` 继续是凭据替换路径；本变更只增加启动和部署入口的失败闭环。
