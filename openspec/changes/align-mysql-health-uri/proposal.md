# Change: 对齐 MySQL URI 健康探测

## Why

MySQL 仓储连接已经支持 `mysql.uri` 优先，但 `/health` 依赖探测仍只使用 `mysql.host` 和 `mysql.port`。当生产或演示环境通过 URI 配置 MySQL 时，健康检查可能探测到错误目标，造成状态误导。

## What Changes

- MySQL 健康探测优先解析 `mysql.uri`，为空时回退到 `host/port`。
- 补充基础测试，确认环境变量可以覆盖 `mysql.uri`。
- 保持 Redis、MongoDB 和 AI 探测逻辑不变。

## Non-Goals

- 不增加数据库认证、schema 或表结构探测。
- 不改变 MySQL 仓储连接字符串格式。