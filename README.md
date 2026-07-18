# IndusPilot

工业智能运维支持平台，面向设备资产、运行监控、告警处理、维护工单和 AI 辅助诊断场景。

## 项目定位

IndusPilot 不是完整 MES、ERP 或 SCADA 替代品，而是一个以 C++/Qt 为核心的工业运维支持系统骨架。当前阶段目标是建立生产级项目边界，让后续模块可以逐步接入。

## 技术方向

- 客户端：C++ / Qt Widgets 起步，保留后续 QML 扩展空间
- 后端：C++ / Linux 优先，当前提供模块化服务骨架
- 数据库：MySQL 存业务主数据，Redis 存会话和运行态，MongoDB 存日志和 AI 上下文
- AI：独立辅助能力，不阻塞核心运维流程

## 当前模块

- 平台骨架
- 用户身份与权限
- 工业资产
- 运行监控
- 告警管理
- 维护工单
- AI 辅助诊断

## 快速入口

- OpenSpec 工作流：`openspec/changes/define-induspilot-foundation`
- 后端代码：`backend/`
- Qt 客户端：`client/`
- 数据库脚本：`database/`
- 部署示例：`deployment/`
- 项目文档：`docs/`

## 生产化后端入口

当前已接入 Drogon HTTP 服务运行时。开发验证命令：

```powershell
cmake --preset dev-http
cmake --build --preset dev-http
ctest --preset dev-http
```

更多说明见 `docs/deployment-backend.md` 和 `docs/development/backend-http-service.md`。