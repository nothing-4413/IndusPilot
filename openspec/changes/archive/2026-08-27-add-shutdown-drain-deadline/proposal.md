# Change: 增加优雅停机排空 deadline

## Why

HTTP shutdown coordinator 当前无限等待在途请求归零。只要某个 handler 或下游依赖永久阻塞，进程就不会退出，编排器无法完成滚动发布或故障恢复。

## What Changes

- 增加可配置的 shutdown drain timeout，支持配置文件和环境变量覆盖。
- 为请求生命周期提供带 deadline 的排空等待结果。
- deadline 到期时记录剩余在途请求并继续停止 HTTP runtime，避免进程永久悬挂。
- 增加配置、生命周期和真实 runtime smoke 覆盖，并更新运维文档。

## Non-Goals

- 本次不强制终止正在执行的业务线程或回收其资源。
- 本次不改变正常排空期间已接受请求继续完成的语义。
