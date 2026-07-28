# 本地工具链状态

当前 Windows 主机工具链状态：

- Visual Studio Build Tools：已安装到默认路径 `C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools`
- MSVC：已验证，`cl` 版本为 `19.44.35228`
- MSBuild：已安装在 Build Tools 默认路径
- CMake：使用 VS Build Tools 自带 CMake，版本 `3.31.6-msvc6`
- Ninja：使用 VS Build Tools 自带 Ninja，版本 `1.12.1`
- Qt：当前使用 Anaconda 中已有 Qt 5.9.7，前缀为 `D:/anaconda/Library`
- vcpkg：当前主机安装到 `C:\Users\20106\vcpkg`，项目 presets 通过 `VCPKG_ROOT` 引用，不依赖固定用户目录
- Redis C++ 客户端：已通过 vcpkg 安装 `redis-plus-plus:x64-windows@1.3.15` 与 `hiredis:x64-windows@1.3.0`
- Drogon HTTP 框架：已通过 vcpkg 安装 `drogon[core,mysql,orm,redis,yaml]:x64-windows@1.9.13#1`

## PATH 与环境变量状态

已将 VS Build Tools 自带的 CMake/Ninja 路径加入当前用户 PATH：

- `C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin`
- `C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja`

已设置用户环境变量：

- `VCPKG_ROOT=C:\Users\20106\vcpkg`

用户级 vcpkg 集成已启用：`vcpkg integrate install`。Visual Studio/MSBuild C++ 项目可以自动包含和链接已安装库；CMake presets 使用 `$env{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake`，其他主机只需设置 `VCPKG_ROOT`。

## 验证命令

Redis session 构建：

```powershell
cmake --preset dev-redis
cmake --build --preset dev-redis
ctest --preset dev-redis
```

Drogon HTTP 服务构建：

```powershell
cmake --preset dev-http
cmake --build --preset dev-http
ctest --preset dev-http
```

## 注意事项

当前项目路径包含中文特殊字符 `！`，CMake/MSVC 可以构建成功，但 qmake/nmake 对该路径的编码显示和相对路径处理较敏感。后续如果遇到 Qt 工具链问题，建议将仓库迁移到纯 ASCII 路径进行构建验证。