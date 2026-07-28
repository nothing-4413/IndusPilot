# Change: 移除 CMakePresets 本机 vcpkg 路径

## Why

`CMakePresets.json` 中的 Redis 和 HTTP preset 写死了 `C:/Users/20106/vcpkg`。其他开发机、CI runner 或演示环境即使已经安装 vcpkg，也会因为用户目录不同而无法直接使用这些 preset，不利于项目生产化和可复现构建。

## What Changes

- 将 Redis/HTTP preset 的 vcpkg toolchain 改为通过 `VCPKG_ROOT` 环境变量定位。
- 将相关测试 preset 的 DLL 搜索路径改为基于 `VCPKG_ROOT` 拼接。
- 部署预检新增 CMakePresets 本机路径检查。
- 更新本地工具链文档，说明 presets 依赖 `VCPKG_ROOT` 而不是固定用户目录。

## Non-Goals

- 不改变 vcpkg triplet、依赖版本或安装方式。
- 不引入 CMakePresets 用户私有文件。
- 不调整 Qt 的 `CMAKE_PREFIX_PATH` 约定。