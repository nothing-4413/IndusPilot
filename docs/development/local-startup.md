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

## 编译提示

当前 Codex 环境缺少 C++ 编译器和 CMake。安装工具链后再执行 CMake 或 qmake 构建。