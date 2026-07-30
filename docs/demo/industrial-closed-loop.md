# 工业告警到工单闭环演示

本演示用于展示 IndusPilot 从运行异常到维护闭环的端到端能力，适合作为 GitHub 项目展示和本地验收脚本。

## 场景

一台泵设备出现高温与振动异常。操作员写入运行状态并触发关键告警，系统派发告警通知，AI 诊断给出风险说明和处置建议。维修员基于告警创建工单，登记现场附件，完成维修后关闭工单和告警。管理员最后导出审计 CSV，用于追踪整条链路。

## 前置条件

先启动 Drogon HTTP 后端：

```powershell
cmake --preset dev-http
cmake --build --preset dev-http
.\build\dev-http\backend\induspilot-backend.exe config\backend.example.yaml
```

默认脚本使用内存仓储和默认账号：

- 管理员：`admin / admin123`
- 操作员：`operator / operator123`
- 维修员：`maintainer / maintainer123`

## 运行

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File demo\industrial-closed-loop.ps1 -BaseUrl "http://127.0.0.1:8080" -OutputDir "demo-output"
```

脚本会生成唯一演示 ID，不会依赖固定资产或告警编号。输出目录包含：

- `00-summary.json`：本次演示的核心 ID 和风险结果
- `01-health.json`：健康检查结果
- `02-asset.json`：演示资产
- `03-runtime-state.json`：运行状态
- `04-alert.json`：关键告警
- `05-notification-dispatch.json`：告警通知派发结果
- `06-ai-diagnosis.json`：AI 诊断结果
- `07-work-order-created.json`：工单创建结果
- `08-work-order-closed.json`：工单关闭结果
- `09-alert-closed.json`：告警关闭结果
- `10-audit-export.csv`：审计导出

## 展示重点

- 角色权限：管理员创建资产，操作员处理告警，维修员处理工单。
- 可观测性：脚本为登录和健康检查传入 trace id，后端响应会回传追踪编号。
- 安全性：登录接口已有失败锁定，演示脚本只使用正常凭据。
- AI 可控性：AI Provider 未启用时仍会产出本地规则诊断；启用 HTTP Provider 后继续记录交互。
- 审计闭环：登录、通知派发和审计导出等关键操作可被查询和导出。