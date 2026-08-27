# 设计：登录审计敏感字段边界

登录成功后使用 `result.session->user.id` 作为审计资源标识，并将资源类型从 `session` 改为 `user`。session token 仍只返回给登录客户端用于后续认证，不进入 audit repository。

```text
login result
   ├── response.data.token ──▶ client authentication
   └── audit.resourceId = user.id ──▶ audit repository
```

失败登录和锁定事件继续使用提交的用户名作为资源标识，因为它不是认证凭据；请求追踪编号继续保留用于关联日志和审计。
