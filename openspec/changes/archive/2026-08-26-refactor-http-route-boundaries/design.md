# 设计：HTTP 模块边界

## 目标结构

```text
http/
├── composition_root
│   ├── 创建 repository、session store 和 service
│   └── 构造 HttpServerContext
├── infrastructure
│   ├── 公共 JSON 响应和错误
│   ├── trace id、日志和 metrics advice
│   └── session / permission guard
├── serialization
│   └── 领域对象到 HTTP JSON/CSV 的转换
└── routes
    ├── platform
    ├── auth
    ├── assets
    ├── monitoring
    ├── alerts
    ├── work_orders
    ├── audit
    └── ai
```

## 依赖方向

```text
route registrar
    │
    ├── HttpServerContext
    ├── HTTP infrastructure helpers
    └── one or more module services
            │
            └── repository interface
```

业务 service 和 domain 层不得包含 Drogon 依赖。跨模块用例继续由 route/application 层显式编排，例如告警转工单；不通过全局单例隐藏依赖。

## 接入契约

每个 route registrar 使用统一入口：

```text
register<Context>Routes(server, context)
```

`HttpServerContext` 持有已经组装好的应用对象和共享服务引用。新增模块只需要增加自己的 registrar、DTO/序列化代码、测试和一次组合根注册，不直接修改其他模块的 handler。

## 迁移策略

1. 先抽取公共 HTTP helper 和上下文，不改变行为。
2. 按业务上下文迁移路由，保持原路径和 handler method 不变。
3. 让 `drogon_server.cpp` 只保留运行时启动、组件组装和 registrar 调用。
4. 继续使用现有 CTest 和 HTTP integration smoke 作为兼容性证明。

中间提交必须可构建；每完成一个上下文迁移，都运行对应测试再提交。
