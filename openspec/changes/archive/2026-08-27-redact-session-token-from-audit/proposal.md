# Change: 移除登录审计中的 session token

## Why

登录成功审计事件当前把真实 session token 写入 `resourceId`。具备审计读取或导出权限的调用方可以看到可复用凭据，违反敏感认证材料不得进入业务日志和审计数据的生产边界。

## What Changes

- 登录成功审计改为记录稳定的用户标识，不再记录 session token。
- 保持审计动作、追踪编号、哈希链和登录响应兼容。
- 增加 HTTP smoke 断言，确保审计查询和 CSV 导出均不包含登录 token。

## Non-Goals

- 本次不改变 session token 的生成、有效期或认证协议。
- 本次不清理已经写入历史数据的 token；历史数据清理由单独的运维流程处理。
