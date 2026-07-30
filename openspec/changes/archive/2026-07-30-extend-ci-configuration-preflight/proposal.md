# Change: 扩展 CI 配置预检覆盖

## Why

后端基础测试运行在 Linux，当前 CMake 只会在 Windows 注册 `deployment/preflight.ps1`，导致 compose 安全基线、CMakePresets 路径规则和 MySQL 迁移幂等性预检没有在 GitHub Actions 中直接执行。配置类问题通常不会触发 C++ 编译失败，需要独立质量门。

## What Changes

- 新增 `configuration preflight` GitHub Actions job，运行在 `windows-latest`。
- CI 直接执行 `deployment/preflight.ps1`，覆盖 compose、迁移、CMakePresets 和配置文件一致性检查。
- CI 通过 `cmake --list-presets=configure` 验证 CMake presets 可解析，并显式提供 `VCPKG_ROOT` 环境变量。

## Non-Goals

- 不在本模块接入真实 MySQL、Redis、MongoDB 服务容器。
- 不构建 Drogon HTTP 或 Qt 客户端目标。
- 不改变现有 backend foundation 和 OpenSpec job 的职责。