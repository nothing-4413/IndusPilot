# IndusPilot

工业智能运维支持平台，面向设备资产、运行监控、告警处理、维护工单和 AI 辅助诊断场景。

## 项目定位

IndusPilot 不是完整 MES、ERP 或 SCADA 替代品，而是一个以 C++/Qt 为核心的工业运维支持系统骨架。当前阶段目标是建立生产级项目边界，让后续模块可以逐步接入。

## 技术方向

- 客户端：C++ / Qt Widgets 客户端，已接入 HTTP 登录、资产列表与状态更新、运行监控列表与状态写入、告警创建/规则/通知投递/列表与处置、维护工单列表、新建/编辑/附件/从告警生成/分派/基础流转、AI 结构化诊断入口和 AI 交互审计查询、分页与 CSV 导出
- 后端：C++ / Drogon HTTP 运行时，模块化服务、仓储接口和统一错误响应已经接入
- 数据库：MySQL 覆盖身份、资产、告警、告警规则、告警通知投递审计、工单、运行状态和 AI 交互审计；Redis 支持会话存储；MongoDB 当前用于依赖健康探测并预留非结构化上下文
- AI：独立辅助诊断能力，支持 disabled/http provider 边界，当前外部推理传输仍为后续扩展

## 当前模块

- 平台骨架
- 用户身份与权限
- 工业资产
- 运行监控
- 告警管理
- 维护工单
- AI 辅助诊断

## 快速入口

- OpenSpec 工作流：`openspec/changes/`
- 后端代码：`backend/`
- Qt 客户端：`client/`
- 数据库脚本：`database/`
- 部署示例：`deployment/`
- 项目文档：`docs/`
- 生产就绪自检：`docs/production-readiness.md`

## 生产化后端入口

当前已接入 Drogon HTTP 服务运行时。开发验证命令：

```powershell
cmake --preset dev-http
cmake --build --preset dev-http
ctest --preset dev-http
```

启动本地依赖后，可通过 `storage.repository_store=mysql` 切换到 MySQL 仓储：

```powershell
cd deployment
docker compose up -d
$env:INDUSPILOT_REPOSITORY_STORE="mysql"
.\build\dev-http\backend\induspilot-backend.exe config\backend.example.yaml
```

更多说明见 `docs/deployment-backend.md`、`docs/development/backend-http-service.md` 和 `docs/production-readiness.md`。

## 操作审计闭环

本阶段新增统一操作审计模块：后端会记录登录成功、告警通知派发和告警通知重试事件，并通过 `GET /api/v1/audit/events` 向具备 `audit:read` 权限的管理员开放查询。Qt 客户端新增“操作审计”导航页，在线模式读取后端审计事件，离线或查询失败时展示本地兜底数据。MySQL 持久化由 `database/mysql/007_operation_audit_events_schema.sql` 提供，并已纳入部署前预检。

## 操作审计筛选分页

操作审计接口支持 `actor`、`action`、`resourceType`、`result` 精确筛选。`GET /api/v1/audit/events` 未传分页参数时保持数组响应；传入 `limit` 或 `offset` 时返回 `{ items, total, limit, offset }`，Qt 客户端“操作审计”页面已接入筛选输入框和上一页/下一页控件。

## 操作审计 CSV 导出

操作审计新增 `audit:export` 独立权限和 `GET /api/v1/audit/events/export` CSV 导出接口，支持复用 `actor`、`action`、`resourceType`、`result` 筛选条件。Qt 客户端“操作审计”页面提供导出按钮，导出成功会在后端记录 `operation-audit.export` 审计事件。
