## ADDED Requirements

### Requirement: CMake presets 不依赖固定用户目录
系统 SHALL 保证提交到仓库的 CMake presets 不写死具体 Windows 用户目录中的 vcpkg 路径，并通过 `VCPKG_ROOT` 定位 vcpkg toolchain 与测试运行时库路径。

#### Scenario: 在不同 Windows 用户目录构建 Redis preset

- **GIVEN** 开发机已设置 `VCPKG_ROOT` 且安装了项目所需 vcpkg 依赖
- **WHEN** 执行 `cmake --preset dev-redis`
- **THEN** CMake SHALL 从 `VCPKG_ROOT` 解析 vcpkg toolchain，而不是依赖 `C:/Users/20106/vcpkg`

#### Scenario: 预检扫描 CMakePresets

- **GIVEN** `CMakePresets.json` 出现 `C:/Users/<name>/vcpkg` 或 `C:\Users\<name>\vcpkg`
- **WHEN** 运行部署预检
- **THEN** 预检 SHALL 失败并提示移除本机 vcpkg 用户路径硬编码