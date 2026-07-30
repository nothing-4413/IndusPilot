# Change: 增加 CI 质量门禁

## Why

当前 CI 已覆盖后端测试、配置预检、依赖冒烟和 OpenSpec 校验，但缺少独立的工程质量门禁来约束 clang 配置、CMake presets 和工作流结构。为了让项目更接近生产级协作，需要把这些工程约束固化为可本地运行、可 CI 阻断的脚本。

## What Changes

- 增加 `.clang-format` 和 `.clang-tidy` 基础配置。
- 增加 `tools/quality/quality_gate.ps1`，检查质量门禁所需文件、CMake presets、CI job 和本机路径硬编码。
- 在 GitHub Actions 中增加 `quality gates` job，安装 clang 工具链并运行严格质量门禁。
- 增加质量门禁开发文档和 README 入口。

## Non-Goals

- 本次不对全仓库 C++ 代码执行批量格式化。
- 本次不把 clang-tidy 运行扩展到完整编译数据库分析。