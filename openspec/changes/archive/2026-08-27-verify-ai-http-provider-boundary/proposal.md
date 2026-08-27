# Change: 验证 AI HTTP provider 运行边界

## Why

项目已经具备 Drogon HTTP provider 实现，但现有文档和自动化验证没有明确区分“请求传输成功”和“远端模型负责生成结构化诊断”。缺少本地可重复的 fake provider，也无法稳定回归鉴权、超时和降级行为。

## What Changes

- 明确 HTTP provider 的请求、响应、超时和降级契约。
- 增加本地 fake HTTP provider 集成 smoke，验证请求 method/path/body、操作头、配置鉴权头和 AI 交互审计。
- 覆盖 HTTP provider 成功、非 2xx、非结构化响应和超时场景，确保核心诊断流程可降级。
- 修正文档中与实际实现不一致的“尚未执行外部推理传输”描述，并说明结构化诊断字段当前仍由本地编排器生成。

## Non-Goals

- 不绑定具体商业模型、SDK 或响应 schema。
- 不让远端 provider 直接决定风险等级、原因或建议动作。
- 不改变 AI API、权限、审计仓储协议或默认 disabled 行为。
