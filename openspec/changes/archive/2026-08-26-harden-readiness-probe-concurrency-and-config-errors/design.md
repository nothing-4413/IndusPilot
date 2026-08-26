# 设计：readiness 探测并发与配置错误

## 配置加载边界

```text
文件/环境变量
       │
       ▼
严格解析（类型、section、key、层级、文件存在性）
       │
       ├── loadErrors 非空 ──▶ validateConfig=false ──▶ 不启动 listener
       │
       └── 无加载错误 ───────▶ 业务范围校验 ───────────▶ 正常启动流程
```

`AppConfig` 保留加载错误集合，`validateConfig` 将其转换为统一的 `ConfigValidation.errors`。类型解析必须消费完整输入；例如 `12ms`、`true-ish` 和空值不能回退到默认值。未知 section/key 和非属性行同样是错误，防止拼写错误被当作默认配置。

环境变量覆盖与文件解析使用相同的严格类型规则。未设置环境变量不产生错误；设置为空字符串视为显式非法值，仅字符串字段允许按现有语义保留为空。

## 探测并发模型

```text
readiness request A ─┐
                      ├─ lock ── cache valid? ── return snapshot
readiness request B ─┘          │
                                └─ probe in flight? wait
                                     │
                                one owner copies config
                                     │ unlock
                         MySQL ─┐
                         Redis ─┼─ parallel probes ── absolute deadline
                         AI ────┘
                                     │ lock
                                publish snapshot + metadata
                                     │ notify_all
```

应用状态锁只保护状态快照、single-flight 标记和计数器；网络探测不持有该锁。探测 owner 发布结果后唤醒等待请求，等待者复用同一结果。停止或未初始化状态不触发探测。

## Deadline

探测开始时计算一个 `steady_clock` absolute deadline。DNS `getaddrinfo`、非阻塞 connect/select 和多地址重试都不能超过该 deadline；超时统一转换为不可用原因。各依赖并行，因此一轮探测的总耗时受单个配置 timeout 控制，而不是所有依赖 timeout 之和。

## 状态观测

readiness 状态增加稳定的 metadata：`probeInProgress`、`probeCount`、`failureCount`、`recoveryCount`、`lastProbeDurationMs` 和 Unix 毫秒时间戳 `lastProbeAtUnixMs`。failure/recovery 按依赖从 available 到 unavailable、以及 unavailable 到 available 的边沿计数；未 required 的依赖不影响 `ready`，但其探测结果仍可诊断。

## 模块边界

配置错误归 app/config；依赖探测、deadline 和结果归 data/data_connectors；single-flight 状态归 app/application；HTTP registrar 只负责序列化 readiness 状态，不直接执行探测。
