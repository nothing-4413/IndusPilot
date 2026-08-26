# 安全门禁

IndusPilot 使用 `tools/security/secret_scan.ps1` 做轻量密钥扫描。脚本只扫描 Git tracked files，避免读取本地未提交的 `deployment/.env` 等真实环境文件。

## 扫描范围

脚本会阻断以下明显风险：

- 被 Git 跟踪的真实 `.env` 文件
- 私钥文件头，例如 `BEGIN PRIVATE KEY`
- GitHub token 常见格式
- OpenAI/API secret key 常见格式
- AWS access key 常见格式

`.env.example` 和文档中的 `change-me-*` 占位值允许存在，用于保留可复现的本地配置说明。

## 本地运行

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File tools/security/secret_scan.ps1
```

CI 中会运行：

```powershell
tools/security/secret_scan.ps1 -FailOnMissingGit
```

## 后续增强

后续接入外部 AI provider、通知渠道或制品发布时，建议继续加入依赖锁文件审计、SBOM、镜像扫描和更完整的密钥历史扫描。