# 任务清单

## 1. OpenSpec
- [x] 1.1 编写 metrics 提案和规格增量
- [x] 1.2 OpenSpec 变更严格校验通过

## 2. 后端指标
- [x] 2.1 增加 MetricsRegistry 聚合 HTTP 与业务指标
- [x] 2.2 Drogon 运行时接入统一 metrics advice
- [x] 2.3 增加 `GET /metrics` Prometheus 文本端点

## 3. 测试与文档
- [x] 3.1 后端基础测试覆盖指标聚合和路径归一化
- [x] 3.2 HTTP smoke 覆盖 `/metrics`
- [x] 3.3 增加可观测性指标文档和 README 入口

## 4. 提交
- [x] 4.1 OpenSpec 全量严格校验通过
- [x] 4.2 后端与客户端 CTest 通过
- [x] 4.3 按模块提交并推送 GitHub