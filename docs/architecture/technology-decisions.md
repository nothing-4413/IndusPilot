# 技术决策记录

## 后端框架

当前生产化阶段选择 Drogon 作为中心后端 HTTP 服务框架。原因是 Drogon 使用 C++14/17，支持异步 HTTP、WebSocket、路由注册、JSON 响应，并且在 vcpkg 中提供 MySQL、Redis、ORM 和 YAML 特性，和 IndusPilot 的 C++/Linux 后端、MySQL、Redis、MongoDB、AI 辅助边界匹配度较高。

Boost.Beast 保留为后续边缘网关、协议适配器或自研网关层候选；它更底层，适合处理定制协议和高控制度网络逻辑，但不作为当前中心业务后端的第一落点。Oat++ 保留为 OpenAPI/DTO 风格服务的备选。

## Qt 客户端

客户端以 Qt Widgets 起步。工业运维系统更依赖密集表格、表单、导航、状态徽标和稳定桌面体验；后续需要更强视觉动效时，可以把局部页面迁移到 QML。

## 构建系统

主构建入口使用 CMake，同时为当前 Windows/Anaconda 环境保留 qmake `.pro` 文件。生产化后端依赖通过 vcpkg 管理，Redis 构建使用 `dev-redis` preset，Drogon HTTP 服务使用 `dev-http` preset。

## 数据库职责

- MySQL：用户、角色、资产、告警、工单、配置等强关系数据。
- Redis：会话、缓存、限流、运行态、队列。
- MongoDB：日志、AI 交互、非结构化诊断资料。

## AI 边界

AI 只作为辅助诊断能力，不承担最终运维决策。所有建议都必须标记为“辅助建议”，核心告警和工单流程不依赖 AI 成功响应。