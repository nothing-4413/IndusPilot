## Why

HTTP AI Provider 已能发起外部推理请求，但缺少生产级鉴权配置和响应契约校验。若 Provider 返回空 JSON、非结构化文本或缺少文本字段，系统不应把它误判为成功推理结果。

## What Changes

- 为 AI HTTP Provider 增加 `api_key`、`auth_header`、`auth_scheme` 配置和环境变量覆盖。
- 请求外部 Provider 时按配置注入鉴权头，不在状态信息中泄露密钥。
- 默认要求 Provider 返回结构化 JSON，并校验可用文本字段。
- Provider 响应缺少文本、非 JSON 或为空时降级为不可用结果，业务流程和 AI 交互记录继续执行。
- 更新示例配置、开发文档和配置加载测试。

## Impact

- 影响模块：ai、config、docs。
- 兼容性：未配置 `api_key` 时不发送鉴权头；可通过 `require_structured_response=false` 兼容纯文本 Provider。