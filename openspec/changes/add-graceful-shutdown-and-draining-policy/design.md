# 设计：优雅停机与请求排空

## 生命周期

```text
                 beginDraining()
 RUNNING ------------------------------> DRAINING
   │                                      │
   │ stop()                               │ all accepted requests complete
   ▼                                      ▼
 STOPPED <--------------------------- stop()
```

`RUNNING` 表示进程已初始化并可以接受请求；`DRAINING` 表示进程仍存活，但不再接受新的业务请求，readiness 固定为 false；`STOPPED` 表示应用资源已停止，liveness 也为 false。`beginDraining()` 和 `stop()` 都必须幂等，重复信号不能重复执行 shutdown。

Application 只保存生命周期状态和可查询快照，不直接依赖 Drogon。`readiness()` 在 `DRAINING` 时不触发外部依赖探测，并返回已有依赖快照与 `ready=false`。

## HTTP 请求排空

```text
request -> Drogon pre-handling gate
             │
             ├─ RUNNING   -> inFlight++ -> handler -> post-handling -> inFlight--
             └─ DRAINING  -> 503 SERVER_DRAINING
```

gate 位于所有业务 handler 之前。健康端点也遵循同一接收边界，但 `/health/live` 和 `/health/ready` 必须允许在 draining 期间返回状态，使编排器能够观察摘流结果；其他端点在 draining 期间返回 `503`。计数在 handler 前增加、post-handling 后减少，计数器和条件变量由 HTTP runtime controller 保护，不持有 Application 状态锁。

正常 shutdown 流程先调用 `beginDraining()`，再停止 listener 接受新连接，等待当前已接受请求计数归零，最后调用 Drogon `quit()` 和 `Application::stop()`。由于 Drogon 的 signal handler 可以从框架线程触发，等待逻辑不能阻塞 Drogon event loop；shutdown coordinator 负责在独立线程中等待并最终退出 event loop。

## 信号和错误

- TERM 和 INT handler 都只发出一次 shutdown 请求，并立即返回。
- listener 绑定失败必须在 `run()` 返回后被识别，打印包含 host/port 的错误，并返回专用非零退出码；不能把失败误报为正常退出。
- 非 Drogon 构建保留现有同步 Application 入口，生命周期 API 仍可由测试和未来 runtime 使用。

## 后续接入边界

HTTP runtime 暴露 `beginDraining`、`waitForDrain` 和 `inFlightRequests` 所需的最小生命周期接口；后续 MySQL、Redis、通知队列和 AI worker 可以在 Application 或 composition root 注册有序 shutdown hook，而无需修改路由文件。
