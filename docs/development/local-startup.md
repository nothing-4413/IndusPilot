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

## 默认后端和 Qt 客户端

默认构建使用内存仓储和内存会话存储，适合快速验证 Qt 客户端和后端业务模块。客户端会读取 `config/client.example.json` 的 `apiBaseUrl`，后端可用时接入 HTTP 登录、资产列表、运行监控列表与状态写入、告警创建/列表与处置、维护工单列表、新建/从告警生成/分派/基础流转、AI 结构化诊断入口和 AI 交互审计查询，后端不可用时保留离线演示数据：

```powershell
cmake -S . -B build/ninja-msvc-client -G Ninja -DINDUSPILOT_BUILD_CLIENT=ON -DCMAKE_PREFIX_PATH="D:/anaconda/Library"
cmake --build build/ninja-msvc-client
ctest --test-dir build/ninja-msvc-client --output-on-failure
```

## Redis 会话存储

Redis C++ 客户端已安装在 `C:\Users\20106\vcpkg`，并设置了用户环境变量 `VCPKG_ROOT`。启用 Redis-backed session 时使用：

```powershell
cmake --preset dev-redis
cmake --build --preset dev-redis
ctest --preset dev-redis
```

## Drogon HTTP 服务

Drogon 已通过 vcpkg 安装，启用真实 HTTP 服务时使用：

```powershell
cmake --preset dev-http
cmake --build --preset dev-http
ctest --preset dev-http
.\build\dev-http\backend\induspilot-backend.exe config\backend.example.yaml
```

启动后可以访问：

- `http://127.0.0.1:8080/health`
- `http://127.0.0.1:8080/api/v1/assets`
- `http://127.0.0.1:8080/api/v1/ai/status`
- `http://127.0.0.1:8080/api/v1/ai/diagnose`

登录示例：

```powershell
Invoke-RestMethod -Uri "http://127.0.0.1:8080/api/v1/auth/login" -Method Post -ContentType "application/json" -Body '{"username":"admin","password":"admin123"}'
```

当前 HTTP 层已经接入结构化配置、会话守卫、权限守卫、统一错误响应、Redis-backed session 选项、MySQL 仓储切换和 AI agent 诊断编排。默认 `storage.repository_store=memory` 仍用于快速开发；验证 MySQL 仓储时先启动 `deployment/docker-compose.yml`，再覆盖：

```powershell
$env:INDUSPILOT_REPOSITORY_STORE="mysql"
.\build\dev-http\backend\induspilot-backend.exe config\backend.example.yaml
```

生产部署前仍需要替换开发口令和演示盐值、补齐登录失败锁定、真实依赖集成测试、监控指标、外部 AI 推理传输，同时补充告警规则/通知联动、工单编辑/附件能力和 AI 审计分页/导出能力。
