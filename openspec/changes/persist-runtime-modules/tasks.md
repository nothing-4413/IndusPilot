# 任务清单

## 1. OpenSpec
- [x] 1.1 编写运行模块持久化变更提案和规格增量
- [x] 1.2 通过 OpenSpec 严格校验

## 2. 仓储接口与实现
- [x] 2.1 为运行监控状态增加仓储接口和内存实现
- [x] 2.2 为告警、工单、运行监控和 AI 交互补齐 MySQL 仓储
- [x] 2.3 补充 MySQL schema 增量

## 3. 服务注入与运行时
- [x] 3.1 将告警服务接入 `AlertRepository`
- [x] 3.2 将维护工单服务接入 `WorkOrderRepository`
- [x] 3.3 将运行监控服务接入 `RuntimeStateRepository`
- [x] 3.4 将 AI 服务接入 `AiInteractionRepository`
- [x] 3.5 让 HTTP 运行时按仓储配置注入运行模块仓储

## 4. 测试与文档
- [x] 4.1 补充仓储注入和配置覆盖测试
- [x] 4.2 更新部署/开发文档中的持久化边界
- [x] 4.3 默认构建和 `dev-http` 测试通过
- [x] 4.4 OpenSpec specs 和 change 严格校验通过

## 5. 提交
- [x] 5.1 按模块提交 Git 记录