param(
    [switch]$RequireDocker
)

$ErrorActionPreference = "Stop"
$script:FailedChecks = 0
$RepoRoot = Split-Path -Parent $PSScriptRoot

function Write-CheckOk {
    param([string]$Message)
    Write-Host "[OK] $Message" -ForegroundColor Green
}

function Write-CheckWarn {
    param([string]$Message)
    Write-Host "[WARN] $Message" -ForegroundColor Yellow
}

function Write-CheckFail {
    param([string]$Message)
    $script:FailedChecks += 1
    Write-Host "[FAIL] $Message" -ForegroundColor Red
}

function Get-RepoPath {
    param([string]$RelativePath)
    return Join-Path $RepoRoot $RelativePath
}

function Test-RequiredFile {
    param([string]$RelativePath)
    $path = Get-RepoPath $RelativePath
    if (Test-Path -LiteralPath $path) {
        Write-CheckOk "存在文件：$RelativePath"
        return $true
    }
    Write-CheckFail "缺少文件：$RelativePath"
    return $false
}

function Get-FileText {
    param([string]$RelativePath)
    $path = Get-RepoPath $RelativePath
    if (-not (Test-Path -LiteralPath $path)) {
        return ""
    }
    return Get-Content -Encoding UTF8 -LiteralPath $path -Raw
}

Write-Host "IndusPilot 部署前预检" -ForegroundColor Cyan
Write-Host "仓库根目录：$RepoRoot"

$requiredFiles = @(
    "config/backend.example.yaml",
    "deployment/docker-compose.yml",
    "database/mysql/001_foundation_schema.sql",
    "database/mysql/002_seed_identity.sql",
    "database/mysql/003_runtime_persistence_schema.sql",
    "database/mysql/004_work_order_attachments_schema.sql",
    "database/mysql/005_alert_rules_notifications_schema.sql",
    "database/mysql/006_alert_notification_delivery_schema.sql",
    "database/mysql/007_operation_audit_events_schema.sql",
    "database/mongodb/init_collections.js"
)

foreach ($file in $requiredFiles) {
    Test-RequiredFile $file | Out-Null
}

$compose = Get-FileText "deployment/docker-compose.yml"
foreach ($service in @("mysql", "redis", "mongodb")) {
    if ($compose -match ("(?m)^\s{{2}}{0}:" -f [regex]::Escape($service))) {
        Write-CheckOk "docker compose 定义服务：$service"
    } else {
        Write-CheckFail "docker compose 未定义服务：$service"
    }
}

$config = Get-FileText "config/backend.example.yaml"
foreach ($fragment in @('repository_store: "memory"', 'session_store: "memory"', 'provider: "disabled"')) {
    if ($config.Contains($fragment)) {
        Write-CheckOk "示例配置包含：$fragment"
    } else {
        Write-CheckFail "示例配置缺少：$fragment"
    }
}

$schema1 = Get-FileText "database/mysql/001_foundation_schema.sql"
if ($schema1 -match "CREATE TABLE IF NOT EXISTS schema_migrations") {
    Write-CheckOk "MySQL schema 版本表已定义"
} else {
    Write-CheckFail "MySQL schema 版本表未定义"
}

$schemaScripts = @(
    "database/mysql/001_foundation_schema.sql",
    "database/mysql/002_seed_identity.sql",
    "database/mysql/003_runtime_persistence_schema.sql",
    "database/mysql/004_work_order_attachments_schema.sql",
    "database/mysql/005_alert_rules_notifications_schema.sql",
    "database/mysql/006_alert_notification_delivery_schema.sql",
    "database/mysql/007_operation_audit_events_schema.sql"
)
$expectedMigrations = @(
    "001_foundation_schema",
    "002_seed_identity",
    "003_runtime_persistence_schema",
    "004_work_order_attachments_schema",
    "005_alert_rules_notifications_schema",
    "006_alert_notification_delivery_schema",
    "007_operation_audit_events_schema"
)
foreach ($migration in $expectedMigrations) {
    $found = $false
    foreach ($scriptPath in $schemaScripts) {
        if ((Get-FileText $scriptPath) -match [regex]::Escape($migration)) {
            $found = $true
            break
        }
    }
    if ($found) {
        Write-CheckOk "schema 版本已登记：$migration"
    } else {
        Write-CheckFail "schema 版本未登记：$migration"
    }
}

$seed = Get-FileText "database/mysql/002_seed_identity.sql"
if ($seed -match 'pbkdf2_sha256\$120000\$') {
    Write-CheckOk "MySQL 演示账号使用 PBKDF2 哈希格式"
} else {
    Write-CheckFail "MySQL 演示账号未使用 PBKDF2 哈希格式"
}
if ($seed -match "ON DUPLICATE KEY UPDATE password_hash = IF") {
    Write-CheckOk "MySQL 种子脚本不会无条件覆盖已有密码哈希"
} else {
    Write-CheckFail "MySQL 种子脚本可能覆盖已有密码哈希"
}

if (Get-Command docker -ErrorAction SilentlyContinue) {
    Write-CheckOk "检测到 docker 命令"
    if ($RequireDocker) {
        Push-Location (Join-Path $RepoRoot "deployment")
        try {
            docker compose config --quiet
            if ($LASTEXITCODE -eq 0) {
                Write-CheckOk "docker compose 配置校验通过"
            } else {
                Write-CheckFail "docker compose 配置校验失败"
            }
        } finally {
            Pop-Location
        }
    } else {
        Write-CheckWarn "未启用 -RequireDocker，跳过 docker compose config 校验"
    }
} elseif ($RequireDocker) {
    Write-CheckFail "启用了 -RequireDocker，但未找到 docker 命令"
} else {
    Write-CheckWarn "未找到 docker 命令；本次仅做离线文件预检"
}

if ($script:FailedChecks -gt 0) {
    Write-Host "预检失败：$script:FailedChecks 项检查未通过" -ForegroundColor Red
    exit 1
}

Write-Host "预检通过：部署基线文件和配置声明完整" -ForegroundColor Green
exit 0