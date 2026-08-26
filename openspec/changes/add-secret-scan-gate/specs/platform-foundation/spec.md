## ADDED Requirements

### Requirement: 密钥扫描安全门禁
系统 SHALL 在仓库质量流程中扫描明显敏感内容，并在 CI 中阻断误提交的密钥材料。

#### Scenario: CI 执行密钥扫描

- **GIVEN** GitHub Actions 收到 push 或 pull request
- **WHEN** 执行安全扫描 job
- **THEN** CI SHALL 运行 `tools/security/secret_scan.ps1 -FailOnMissingGit`
- **AND** 检测到真实 `.env`、私钥头或常见 token 模式时 SHALL 阻断合入

#### Scenario: 本地运行密钥扫描

- **GIVEN** 开发者位于仓库根目录
- **WHEN** 开发者运行 `tools/security/secret_scan.ps1`
- **THEN** 脚本 SHALL 扫描 Git tracked files
- **AND** 脚本 SHALL 输出中文通过或失败信息

#### Scenario: 示例密钥不会误报

- **GIVEN** 仓库包含 `.env.example` 或文档中的 `change-me-*` 示例值
- **WHEN** 密钥扫描运行
- **THEN** 扫描 SHALL 允许这些占位示例
- **AND** 仍 SHALL 阻止真实 `.env` 文件被提交

#### Scenario: 质量门禁保护安全扫描入口

- **WHEN** 开发者运行 `tools/quality/quality_gate.ps1`
- **THEN** 质量门禁 SHALL 检查密钥扫描脚本存在
- **AND** 质量门禁 SHALL 检查 CI 包含 security scan job