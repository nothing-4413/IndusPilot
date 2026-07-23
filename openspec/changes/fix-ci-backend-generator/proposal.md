## 背景

GitHub Actions 中的 `CI/后端基础架构` 作业使用 `windows-latest` 并写死 `Visual Studio 17 2022` 生成器。当前远端 runner 在配置阶段报告找不到 Visual Studio 实例，导致后端基础测试未能执行。

## 变更内容

- 将后端基础 CI 作业切换为不依赖 Visual Studio 的 Linux 构建路径。
- 使用 CMake 默认 Unix Makefiles 生成器构建后端基础测试。
- 保留 OpenSpec 校验作业不变。

## 影响范围

- 仅影响 GitHub Actions 基础 CI 的执行环境和 CMake 配置命令。
- 不改变产品代码、后端接口或客户端行为。
- 本地 Windows 开发构建和 `dev-http` preset 不受影响。
