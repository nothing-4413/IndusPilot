# 设计：优雅停机排空 deadline

## 配置边界

新增 `shutdown.drain_timeout_ms`，默认 10000 ms；环境变量 `INDUSPILOT_SHUTDOWN_DRAIN_TIMEOUT_MS` 具有同等严格整数解析规则。值必须大于零，非法值在 listener 启动前阻止启动。

## 排空流程

```text
SIGTERM/SIGINT
      │
      ▼
stopAccepting + beginDraining
      │
      ▼
waitForDrainFor(timeout)
      ├── 全部完成 ──▶ quit
      └── 超时 ──────▶ 记录剩余请求 ──▶ quit
```

deadline 只限制 coordinator 等待时间；它不强制取消 handler。这样可以避免主进程永久等待，同时保留已接受请求的正常完成机会。

## 兼容性

- 保留无参数 `waitForDrain()` 供现有调用方使用。
- `shutdown.drain_timeout_ms` 只影响收到停机信号后的等待上限。
- 正常排空完成时日志语义保持不变；超时时追加明确的 timeout 日志。
