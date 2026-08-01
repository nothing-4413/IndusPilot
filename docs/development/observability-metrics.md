# 可观测性指标

IndusPilot 后端在启用 Drogon HTTP 运行时时提供 Prometheus 文本格式指标端点：

```text
GET /metrics
```

该端点当前不需要业务会话，便于本地 Prometheus、CI smoke 和演示脚本直接抓取。生产部署时建议由网关、反向代理或内网网络策略限制访问范围。

## 指标

- `induspilot_http_requests_total`：后端已处理 HTTP 请求总数。
- `induspilot_http_errors_total`：HTTP 状态码大于等于 400 的响应总数。
- `induspilot_ai_requests_total`：AI 辅助请求总数，不包含 AI 状态查询和交互审计查询。
- `induspilot_alert_closures_total`：成功关闭告警的次数。
- `induspilot_work_order_closures_total`：成功关闭维护工单的次数。
- `induspilot_http_route_requests_total{method,path,status}`：按方法、归一化路径和状态码聚合的请求数。
- `induspilot_http_route_duration_ms_sum{method,path,status}`：按方法、归一化路径和状态码聚合的请求耗时毫秒总和。
- `induspilot_http_route_duration_ms_count{method,path,status}`：请求耗时样本数。

## 路径归一化

指标会把包含业务编号的路径归一化，例如：

```text
/api/v1/assets/asset-001/status -> /api/v1/assets/{id}/status
/api/v1/work-orders/wo-001/close -> /api/v1/work-orders/{id}/close
```

这样可以避免资产编号、告警编号和工单编号进入 label，降低 Prometheus 高基数风险。

## 验证

HTTP smoke 测试会访问 `/metrics`，并检查总请求、错误请求、AI 请求和归一化业务路由是否出现在输出中。