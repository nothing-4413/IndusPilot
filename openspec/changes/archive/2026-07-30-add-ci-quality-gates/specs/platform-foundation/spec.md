## ADDED Requirements

### Requirement: CI 质量门禁
系统 SHALL 在 CI 中执行独立的质量门禁，验证工程格式化配置、静态分析配置、CMake presets 和工作流基础约束。

#### Scenario: CI 运行质量门禁
- **GIVEN** GitHub Actions 收到 push 或 pull request
- **WHEN** 执行质量门禁 job
- **THEN** CI SHALL 校验 `.clang-format`、`.clang-tidy`、`CMakePresets.json` 和工作流结构
- **AND** CI SHALL 在缺少 clang 工具链或基础约束失败时阻断合入

#### Scenario: 开发者本地运行质量门禁
- **GIVEN** 开发者在仓库根目录运行质量门禁脚本
- **WHEN** 本地未安装 clang 工具链
- **THEN** 脚本 SHALL 完成仓库结构和配置约束检查，并提示本地跳过 clang 工具版本检查