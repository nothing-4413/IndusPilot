# Change: 增加密钥扫描安全门禁

## Why

项目已经包含真实依赖、运行时验收脚本和 CI 工作流，后续接入更多外部服务时，误提交 `.env`、私钥或 API token 的风险会增加。为了提升生产级工程可信度，需要在仓库级质量门禁中加入轻量密钥扫描，并在 CI 中独立阻断明显敏感内容。

## What Changes

- 增加 `tools/security/secret_scan.ps1`，基于 Git tracked files 扫描敏感文件和常见 token 模式。
- CI 新增 `security scan` job，推送和 PR 时运行密钥扫描。
- 质量门禁检查密钥扫描脚本和 CI job 是否存在。
- 增加安全门禁文档，说明扫描范围、允许的示例配置和本地运行方式。

## Non-Goals

- 本次不引入外部商业密钥扫描服务。
- 本次不扫描未纳入 Git 跟踪的本地文件内容。