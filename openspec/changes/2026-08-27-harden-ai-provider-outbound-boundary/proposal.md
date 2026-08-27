# Change: 收紧 AI provider 外部调用边界

## Why

HTTP provider 已能受控发起请求，但当前同步调用没有重试上限、响应大小上限或 prompt/context 的敏感键值脱敏。生产环境中这会放大瞬时依赖故障、消耗无界内存，并可能把凭据样式内容送入外部服务或 AI 审计记录。

## What Changes

- 增加有限的 HTTP provider 重试配置，并让所有尝试共享一次总超时 deadline。
- 增加响应字节上限，超限响应直接降级，不进入解析或审计输出。
- 对 outbound prompt/context 和 AI 交互输入中的常见 `token`、`password`、`secret`、`api_key`、`authorization`、`credential` 键值做稳定脱敏。
- 扩展 fake provider smoke，覆盖重试成功、总超时、响应大小限制和敏感值不外传。

## Non-Goals

- 不实现模型语义级别的 PII 检测或内容安全审核。
- 不重试非瞬时的 4xx 响应，不改变 provider API 或本地诊断编排。
- 不将 provider 原始响应变成权威结构化命令。
