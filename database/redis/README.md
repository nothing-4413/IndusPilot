# Redis 会话与运行状态缓存

## 客户端依赖

Windows 本机已通过 vcpkg 安装 Redis C++ 客户端：

- vcpkg 根目录：`C:\Users\20106\vcpkg`
- C++ 客户端：`redis-plus-plus:x64-windows@1.3.15`
- C 客户端依赖：`hiredis:x64-windows@1.3.0`
- 用户环境变量：`VCPKG_ROOT=C:\Users\20106\vcpkg`

启用 Redis 会话存储时，CMake 需要使用 vcpkg toolchain：

```powershell
cmake -S . -B build/ninja-msvc-redis -G Ninja `
  -DCMAKE_TOOLCHAIN_FILE="C:/Users/20106/vcpkg/scripts/buildsystems/vcpkg.cmake" `
  -DINDUSPILOT_WITH_REDIS=ON `
  -DINDUSPILOT_BUILD_CLIENT=OFF
```

## 键空间规划

会话键使用 `induspilot:session:{token}`，值为后端内部长度前缀序列化结构，过期时间由 Redis TTL 控制。登出时删除对应 session key；验证时如果 Redis 中不存在该键或 TTL 已到期，则要求用户重新认证。

后续模块建议继续沿用以下前缀：

- `induspilot:cache:*`：低频主数据缓存。
- `induspilot:state:*`：设备运行状态快照。
- `induspilot:queue:*`：告警、AI 分析、工单流转队列。

## 当前边界

后端配置会解析 `redis.password` 和 `redis.database`，但当前 Redis session 连接实现使用 `redis.uri` 直接创建客户端。需要认证或选择 DB 时，请把口令和 DB 写入 `redis.uri`，例如 `tcp://:password@127.0.0.1:6379/0`。
