# 本地开发启动

## 依赖服务

```powershell
cd deployment
docker compose up -d
```

启动后会暴露：

- MySQL：`127.0.0.1:3306`
- Redis：`127.0.0.1:6379`
- MongoDB：`127.0.0.1:27017`

## 配置文件

复制示例配置后再调整本机密码和端口：

```powershell
copy config/backend.example.yaml config/backend.yaml
copy config/client.example.json config/client.json
copy config/ai.example.yaml config/ai.yaml
```

## 编译后端和客户端

默认构建使用内存会话存储，适合快速测试：

```powershell
cmake -S . -B build/ninja-msvc-client -G Ninja -DINDUSPILOT_BUILD_CLIENT=ON -DCMAKE_PREFIX_PATH="D:/anaconda/Library"
cmake --build build/ninja-msvc-client
ctest --test-dir build/ninja-msvc-client --output-on-failure
```

## 启用 Redis 会话存储

Redis C++ 客户端已安装在 `C:\Users\20106\vcpkg`，并设置了用户环境变量 `VCPKG_ROOT`。启用 Redis-backed session 时使用：

```powershell
cmake -S . -B build/ninja-msvc-redis -G Ninja `
  -DCMAKE_TOOLCHAIN_FILE="C:/Users/20106/vcpkg/scripts/buildsystems/vcpkg.cmake" `
  -DINDUSPILOT_WITH_REDIS=ON `
  -DINDUSPILOT_BUILD_CLIENT=OFF
cmake --build build/ninja-msvc-redis
```

后端代码中的 `RedisSessionStore` 使用 `redis-plus-plus` 连接 Redis，例如：`tcp://127.0.0.1:6379`。生产环境应从配置文件读取 URI、连接池、安全认证和会话 TTL。