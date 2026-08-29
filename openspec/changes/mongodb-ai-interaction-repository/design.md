# Design: MongoDB AI 交互仓储

## Storage Selection

新增 `storage.ai_interaction_store`，默认 `memory`。当值为 `mysql` 时保留现有行为；当值为 `mongodb` 时仅替换 `AiInteractionRepository`。`storage.repository_store` 继续控制事务型仓储，避免一次配置同时改变多个数据边界。

## MongoDB Adapter

使用 MongoDB C++ Driver `mongocxx`，在 adapter 内通过 URI 创建 client，选择配置的 database 和 `ai_interactions` collection。`interactionCode` 为唯一业务键；save 使用 replace/upsert，list 使用关联对象过滤并按 `createdAt` 降序。Mongo 文档字段与现有初始化脚本保持一致：`interactionCode`、`relatedType`、`relatedId`、`prompt`、`response`、`createdAt`、`updatedAt`。

## Runtime and Failure Handling

HTTP context 创建 Mongo adapter 时使用配置 URI；未启用 Mongo store 时不创建 client。Mongo adapter 的连接和命令异常向 AI service 传播，由既有路由错误边界处理，不伪造成功审计记录。健康检查在 Mongo store 被选中时将 Mongo 标记为 required，并使用既有依赖探测结果。

## Compatibility

没有 Mongo C++ driver 的 foundation 构建继续可用；Mongo 代码仅在 `INDUSPILOT_WITH_MONGODB` 下编译。默认配置仍使用 memory，因此现有 CTest 和 Qt 演示不依赖 MongoDB。
