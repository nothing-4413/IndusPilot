# Change: 增加优雅停机与请求排空策略

## Why

当前 HTTP runtime 只在 `run()` 返回后调用 `Application::stop()`，没有定义信号触发后的生命周期，也没有将实例从流量准入中摘除。生产编排器发送 `SIGTERM` 或操作员按下 `Ctrl+C` 时，进程可能直接停止网络循环，已有请求的完成、readiness 状态和退出码都没有稳定契约。

## What Changes

- 为 Application 增加稳定的运行、draining 和 stopped 生命周期状态。
- 停机开始时立即使 readiness 返回 `503`，同时保持 liveness 可用，直到进程真正停止。
- 在 HTTP runtime 增加统一请求排空 gate：draining 开始后拒绝新业务请求，已进入处理阶段的请求继续完成。
- 通过 Drogon 的 TERM/INT handler 触发同一套 shutdown 流程，避免信号路径和手动路径行为分叉。
- 为 listener 启动失败提供明确的非零退出码和 stderr 错误信息。
- 增加生命周期单元测试、HTTP shutdown smoke 和部署操作说明。

## Non-Goals

- 不改变业务 API、认证模型、仓储协议和正常请求响应 envelope。
- 不在本 change 中实现进程管理器、容器编排器或跨进程 leader election。
- 不强制终止超过排空期限的业务请求；超时强制退出作为后续独立策略评估。
- 不把外部依赖关闭细节塞入 HTTP route registrar；后续模块通过显式 shutdown hook 接入。
