## Why

登录接口当前只返回统一认证失败，缺少针对短时间连续失败尝试的锁定与限流反馈。工业支持系统需要在不泄露账号存在性的前提下抑制暴力登录，并将失败和锁定事件纳入审计链路。

## What Changes

- 在身份服务中加入可配置登录失败窗口、失败阈值和锁定时长。
- 登录达到锁定阈值后返回 `AUTHENTICATION_LOCKED`，HTTP 层映射为 `429 Too Many Requests` 并回传 `Retry-After`。
- 登录失败和锁定事件写入操作审计，便于安全追踪。
- 补充 C++ 单测、HTTP smoke、示例配置和验证记录。

## Impact

- 影响模块：identity、http、config、operation-audit。
- 兼容性：默认启用，阈值可通过配置文件和环境变量调整；正常登录路径保持不变。