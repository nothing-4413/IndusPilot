# 设计：身份凭据生命周期

## 边界

```text
authenticated request
        │
        ▼
validate session + resolve username
        │
        ▼
verify current password
        │
        ├─ invalid ── audit failure, return 401/400
        │
        ▼
validate new password policy
        │
        ▼
generate random salt + PBKDF2-SHA256 hash
        │
        ▼
repository.updatePasswordHash
        │
        ▼
audit success, keep current session active
```

## Password policy

`security.password_min_length` 和 `security.password_iterations` 从 YAML 与环境变量加载。最小长度默认 12，迭代次数默认 120000；校验拒绝小于 8 的长度和小于 100000 的迭代次数。策略校验在 listener 启动前完成。

## Hashing

新增哈希生成函数使用 `std::random_device` 生成 16 字节盐，并输出已有的 `pbkdf2_sha256$iterations$salt$hash` 格式。校验仍支持 `plain:` 和早期无版本格式，以保证内存演示和旧数据可读；新密码永远不写入兼容格式。

## HTTP contract

`POST /api/v1/auth/password` 要求 Bearer session，并接收 JSON `{\"currentPassword\":\"...\",\"newPassword\":\"...\"}`。当前密码错误返回通用认证失败，不暴露用户存在性；新密码不符合策略返回明确的无效请求。成功返回统一 envelope，响应、日志和审计不包含明文密码或哈希。

## Audit

成功记录 `auth.password.changed`，失败记录 `auth.password.change.failed`。resource type 为 `user`，resource id 使用当前用户 id，result 只使用 `success`、`invalid_current_password`、`invalid_new_password` 或 `storage_failed`。现有 trace id 继续贯穿事件。

## Compatibility

内存仓储和 MySQL 仓储都实现同一个 `updatePasswordHash` 接口。改密后当前 session 保持有效；session 全量撤销需要 Redis 按用户索引，作为后续独立设计处理。
