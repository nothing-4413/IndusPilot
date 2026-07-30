# Change: 增加 Agent 工作流展示

## Why

项目已经具备 OpenSpec、模块提交、CI 和工业闭环演示，但缺少一个面向展示的证据入口来说明 agent 如何完成需求到交付的闭环。为了让 GitHub 项目更清晰地体现 agent 能力，需要增加可生成的工作流证据包和说明文档。

## What Changes

- 增加 Agent 交付闭环文档，说明 OpenSpec、实现、验证、提交和 CI 的协作关系。
- 增加 `demo/agent-workflow-evidence.ps1`，生成结构化 JSON 和 Markdown 证据包。
- README 增加 Agent 工作流展示入口。

## Non-Goals

- 本次不新增外部模型 SDK。
- 本次不替换现有工业闭环演示脚本。