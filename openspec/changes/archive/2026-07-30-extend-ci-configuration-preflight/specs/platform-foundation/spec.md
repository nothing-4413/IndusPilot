## ADDED Requirements

### Requirement: CI 覆盖配置预检
系统 SHALL 在 GitHub Actions 中执行独立的配置预检质量门，确保部署配置、迁移脚本和 CMake presets 的仓库级约束不会只停留在本地验证。

#### Scenario: 推送配置变更

- **GIVEN** 提交修改了 compose、迁移脚本、CMake presets 或示例配置
- **WHEN** GitHub Actions 运行 CI
- **THEN** CI SHALL 执行 `deployment/preflight.ps1` 并在预检失败时阻断合入

#### Scenario: presets 语法或宏不可解析

- **GIVEN** 提交修改了 `CMakePresets.json`
- **WHEN** GitHub Actions 运行配置预检 job
- **THEN** CI SHALL 执行 `cmake --list-presets=configure` 并在 presets 无法解析时失败