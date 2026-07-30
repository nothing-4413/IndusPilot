# Change: 身份密码校验生产化

## Why

当前身份服务已经接入仓储，但密码字段仍按明文比较，MySQL 种子也保留 `CHANGE_ME_HASH` 占位。这会让 `repository_store=mysql` 的演示链路无法真实登录，也无法展示生产级密码策略边界。

## What Changes

- 增加版本化密码校验器，支持 `pbkdf2_sha256$iterations$salt$hash` 格式。
- 保留 `plain:` 和早期明文兼容，避免本地演示和历史测试数据一次性失效。
- 让 `IdentityService` 通过统一密码校验边界完成登录和认证。
- 将 MySQL 演示账号种子更新为 PBKDF2 哈希，并补齐 operator/maintainer 角色账号。
- 更新文档，明确演示哈希与生产密码策略的差异。

## Non-Goals

- 不在本次实现 Argon2/bcrypt 依赖接入。
- 不在本次实现登录失败锁定、密码轮换、重置流程或审计事件表。
- 不引入外部 IAM、LDAP、OAuth 或 SSO。