# 质量门禁

IndusPilot 使用独立质量门禁脚本和 GitHub Actions job 保护工程基础约束，目标是让格式配置、静态分析配置、CMake presets、OpenSpec 校验和部署预检都能在 CI 中被持续验证。

## 本地执行

Windows 开发环境可以直接运行：

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File tools/quality/quality_gate.ps1
```

本地脚本默认不强制要求安装 `clang-format` 和 `clang-tidy`。如果开发机已安装对应工具，脚本会输出版本信息。

## CI 执行

GitHub Actions 的 `quality gates` job 会安装 clang 工具链，并使用严格模式运行：

```powershell
pwsh -NoProfile -File tools/quality/quality_gate.ps1 -RequireClangTools
```

该 job 会阻断以下问题：

- 缺少 `.clang-format` 或 `.clang-tidy`。
- CMake presets 缺少 `dev`、`dev-redis`、`dev-http` 的 configure/build/test 定义。
- `CMakePresets.json` 写死本机用户目录中的 vcpkg 路径。
- CI 工作流缺少质量门禁、OpenSpec、后端测试、配置预检或依赖冒烟 job。

后续如果对全仓库进行格式化，可以在该脚本中进一步打开 `clang-format --dry-run` 和 `clang-tidy` 编译数据库检查。