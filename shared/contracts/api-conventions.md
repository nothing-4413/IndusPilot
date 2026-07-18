# API 约定

## 响应格式

```json
{
  "success": true,
  "code": "OK",
  "message": "操作成功",
  "data": {}
}
```

## 分页格式

```json
{
  "page": 1,
  "pageSize": 20,
  "total": 100
}
```

## 错误分类

- `AUTH_REQUIRED`：需要登录
- `PERMISSION_DENIED`：权限不足
- `VALIDATION_FAILED`：输入校验失败
- `RESOURCE_NOT_FOUND`：资源不存在
- `DEPENDENCY_UNAVAILABLE`：外部依赖不可用
- `AI_UNAVAILABLE`：AI 服务不可用