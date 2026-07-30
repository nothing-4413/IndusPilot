# Agent 交付闭环

本页用于说明 IndusPilot 如何展示 agent 型工程能力。项目不只包含业务代码，还保留从需求提出、规格演进、实现验证、Git 提交到 CI 反馈的连续证据。

## 闭环链路

```mermaid
flowchart LR
    A["需求或建议"] --> B["OpenSpec change"]
    B --> C["模块实现"]
    C --> D["本地质量门禁"]
    D --> E["OpenSpec/CTest 验证"]
    E --> F["按模块 Git 提交"]
    F --> G["GitHub Actions"]
    G --> H["归档为正式规格"]
```

## 可展示能力

- 需求澄清：通过 OpenSpec proposal/spec/tasks 固化模块目标、范围和验收条件。
- 编排执行：按模块实现 C++ 后端、Qt 客户端、数据库、部署、脚本和文档。
- 质量验证：质量门禁、OpenSpec、CTest、HTTP smoke 和依赖 smoke 共同形成验证证据。
- 可追溯交付：每个模块使用独立 Git 提交，并推送触发 GitHub Actions。
- 工业 Agent 场景：告警、资产状态、工单历史和操作员描述进入 AI 诊断编排，输出风险等级、可能原因、建议动作和审计记录。

## 证据包生成

运行以下命令可生成本地证据包：

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File demo\agent-workflow-evidence.ps1
```

默认输出目录为 `build/agent-workflow-evidence`，包含：

- `agent-workflow-evidence.json`：结构化证据，适合后续上传、归档或接入展示页。
- `agent-workflow-evidence.md`：面向展示和答辩的 Markdown 摘要。

如果已经运行过工业闭环演示，脚本会自动读取 `build/demo-closed-loop-output/00-summary.json` 并纳入证据包。

## 展示话术

IndusPilot 的 agent 能力不等同于简单调用模型接口。项目重点展示的是：agent 根据规格推进模块、执行验证、保留审计和交付证据，并在工业场景中把多源上下文组织成可审计、可降级、需人工复核的诊断结果。