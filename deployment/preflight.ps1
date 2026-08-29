param(
    [switch]$RequireDocker,
    [switch]$RequireProductionSecrets
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

function Get-DotEnvValues {
    param([string]$RelativePath)
    $path = Get-RepoPath $RelativePath
    if (-not (Test-Path -LiteralPath $path)) {
        return $null
    }
    $values = @{}
    foreach ($line in Get-Content -LiteralPath $path -Encoding UTF8) {
        $trimmed = $line.Trim()
        if ([string]::IsNullOrWhiteSpace($trimmed) -or $trimmed.StartsWith('#')) {
            continue
        }
        $separator = $trimmed.IndexOf('=')
        if ($separator -gt 0) {
            $values[$trimmed.Substring(0, $separator).Trim()] = $trimmed.Substring($separator + 1).Trim().Trim('"')
        }
    }
    return $values
}

function Test-ProductionSecretValue {
    param([hashtable]$Values, [string]$Name)
    if (-not $Values.ContainsKey($Name) -or [string]::IsNullOrWhiteSpace($Values[$Name])) {
        Write-CheckFail "生产环境缺少非空密钥：$Name"
        return
    }
    if ($Values[$Name].Trim().ToLowerInvariant().StartsWith('change-me')) {
        Write-CheckFail "生产环境仍使用示例密钥：$Name"
        return
    }
    Write-CheckOk "生产环境密钥已配置：$Name"
}

Write-Host "IndusPilot 部署前预检" -ForegroundColor Cyan
Write-Host "仓库根目录：$RepoRoot"

$requiredFiles = @(
    "config/backend.example.yaml",
    "deployment/docker-compose.yml",
    "deployment/.env.example",
    "database/mysql/001_foundation_schema.sql",
    "database/mysql/002_seed_identity.sql",
    "database/mysql/003_runtime_persistence_schema.sql",
    "database/mysql/004_work_order_attachments_schema.sql",
    "database/mysql/005_alert_rules_notifications_schema.sql",
    "database/mysql/006_alert_notification_delivery_schema.sql",
    "database/mysql/007_operation_audit_events_schema.sql",
    "database/mysql/008_operation_audit_export_permission.sql",
    "database/mysql/009_operation_audit_integrity_schema.sql",
    "database/mysql/010_redact_legacy_login_audit_tokens.sql",
    "database/mysql/011_credential_version.sql",
    "database/mysql/012_seed_account_governance.sql",
    "database/mysql/013_notification_delivery_queue.sql",
    "database/mysql/014_migration_integrity.sql",
    "database/mysql/015_audit_siem_delivery_queue.sql",
    "database/mysql/migrate.sh",
    "database/mongodb/init_collections.js",
    "database/mongodb/integration/real_crud_smoke.js",
    "backend/tests/http_runtime_profile_smoke.ps1",
    "backend/tests/mysql_migration_runner_smoke.sh",
    "deployment/rotate_seed_credentials.ps1"
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

foreach ($secret in @("INDUSPILOT_MYSQL_ROOT_PASSWORD", "INDUSPILOT_MYSQL_PASSWORD", "INDUSPILOT_REDIS_PASSWORD", "INDUSPILOT_MONGODB_ROOT_PASSWORD")) {
    if ($compose.Contains('${' + $secret + ':?')) {
        Write-CheckOk "docker compose 要求显式配置密钥：$secret"
    } else {
        Write-CheckFail "docker compose 未强制配置密钥：$secret"
    }
}
foreach ($binding in @("INDUSPILOT_MYSQL_BIND:-127.0.0.1", "INDUSPILOT_REDIS_BIND:-127.0.0.1", "INDUSPILOT_MONGODB_BIND:-127.0.0.1")) {
    if ($compose.Contains('${' + $binding + '}')) {
        Write-CheckOk "docker compose 默认仅绑定本地回环：$binding"
    } else {
        Write-CheckFail "docker compose 缺少本地回环绑定默认值：$binding"
    }
}
$healthcheckCount = ([regex]::Matches($compose, "(?m)^\s{4}healthcheck:")).Count
if ($healthcheckCount -ge 3) {
    Write-CheckOk "docker compose 为核心依赖定义 healthcheck"
} else {
    Write-CheckFail "docker compose healthcheck 数量不足"
}
if ($compose.Contains('../database/mongodb:/docker-entrypoint-initdb.d:ro')) {
    Write-CheckOk "MongoDB 初始化目录挂载包含 integration smoke 脚本"
} else {
    Write-CheckFail "MongoDB 初始化目录未挂载，integration smoke 脚本无法在容器中运行"
}

$config = Get-FileText "config/backend.example.yaml"
foreach ($fragment in @('repository_store: "memory"', 'ai_interaction_store: "memory"', 'session_store: "memory"', 'provider: "disabled"')) {
    if ($config.Contains($fragment)) {
        Write-CheckOk "示例配置包含：$fragment"
    } else {
        Write-CheckFail "示例配置缺少：$fragment"
    }
}

$cmakePresets = Get-FileText "CMakePresets.json"
if ($cmakePresets -match "(?i)C:[/\\]Users[/\\][^/\\]+[/\\]vcpkg") {
    Write-CheckFail "CMakePresets.json 不应写死本机 vcpkg 用户路径"
} else {
    Write-CheckOk "CMakePresets.json 未写死本机 vcpkg 用户路径"
}
if ($cmakePresets.Contains('$env{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake')) {
    Write-CheckOk "CMake presets 使用 VCPKG_ROOT 定位 toolchain"
} else {
    Write-CheckFail "CMake presets 未使用 VCPKG_ROOT 定位 toolchain"
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
    "database/mysql/007_operation_audit_events_schema.sql",
    "database/mysql/008_operation_audit_export_permission.sql",
    "database/mysql/009_operation_audit_integrity_schema.sql",
    "database/mysql/010_redact_legacy_login_audit_tokens.sql",
    "database/mysql/011_credential_version.sql",
    "database/mysql/012_seed_account_governance.sql",
    "database/mysql/013_notification_delivery_queue.sql",
    "database/mysql/014_migration_integrity.sql",
    "database/mysql/015_audit_siem_delivery_queue.sql"
)
$expectedMigrations = @(
    "001_foundation_schema",
    "002_seed_identity",
    "003_runtime_persistence_schema",
    "004_work_order_attachments_schema",
    "005_alert_rules_notifications_schema",
    "006_alert_notification_delivery_schema",
    "007_operation_audit_events_schema",
    "008_operation_audit_export_permission",
    "009_operation_audit_integrity_schema",
    "010_redact_legacy_login_audit_tokens",
    "011_credential_version",
    "012_seed_account_governance",
    "013_notification_delivery_queue",
    "014_migration_integrity",
    "015_audit_siem_delivery_queue"
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

$migrationRunner = Get-FileText "database/mysql/migrate.sh"
if ($migrationRunner -match "schema_migrations" -and $migrationRunner -match "ORDER BY version" -and
    $migrationRunner -match "unknown recorded migration version" -and $migrationRunner -match "migration order gap" -and
    $migrationRunner -match "GET_LOCK" -and $migrationRunner -match "migration checksum mismatch") {
    Write-CheckOk "MySQL 迁移入口包含版本、锁和完整性阻止"
} else {
    Write-CheckFail "MySQL 迁移入口缺少版本、锁或完整性治理检查"
}

$ciWorkflow = Get-FileText ".github/workflows/ci.yml"
$runtimeProfileSmoke = Get-FileText "backend/tests/http_runtime_profile_smoke.ps1"
$customDatabaseOverride = "INDUSPILOT_MYSQL_DATABASE=induspilot_ci_custom_db"
if (([regex]::Matches($ciWorkflow, [regex]::Escape($customDatabaseOverride))).Count -ge 2 -and
    ([regex]::Matches($ciWorkflow, "INDUSPILOT_EXPECTED_MYSQL_DATABASE:\s+induspilot_ci_custom_db")).Count -ge 2 -and
    $runtimeProfileSmoke.Contains('-MySqlDatabase $mysqlDatabase')) {
    Write-CheckOk "CI 在依赖和运行时 profile 中覆盖并验证自定义 MySQL 数据库名"
} else {
    Write-CheckFail "CI 未完整覆盖自定义 MySQL 数据库名契约"
}

$hardCodedDatabaseSelection = $false
foreach ($scriptPath in $schemaScripts + @("database/mysql/integration/real_crud_smoke.sql")) {
    if ((Get-FileText $scriptPath) -match "(?im)^\s*USE\s+induspilot\s*;") {
        $hardCodedDatabaseSelection = $true
        break
    }
}
if ($hardCodedDatabaseSelection) {
    Write-CheckFail "MySQL SQL 脚本仍硬编码 USE induspilot"
} else {
    Write-CheckOk "MySQL SQL 脚本使用迁移入口选择的数据库"
}

foreach ($scriptPath in $schemaScripts) {
    $scriptText = Get-FileText $scriptPath
    $usesColumnAlter = $scriptText -match "(?is)ALTER\s+TABLE.*ADD\s+COLUMN"
    $checksColumns = $scriptText -match "INFORMATION_SCHEMA\.COLUMNS" -or $scriptText -match "(?is)ADD\s+COLUMN\s+IF\s+NOT\s+EXISTS"
    if ($usesColumnAlter -and -not $checksColumns) {
        Write-CheckFail "MySQL 迁移缺少列幂等保护：$scriptPath"
    } elseif ($usesColumnAlter) {
        Write-CheckOk "MySQL 列迁移具备幂等保护：$scriptPath"
    }

    $usesIndexAlter = $scriptText -match "(?is)ALTER\s+TABLE.*ADD\s+(UNIQUE\s+)?INDEX"
    $checksIndexes = $scriptText -match "INFORMATION_SCHEMA\.STATISTICS" -or $scriptText -match "(?is)ADD\s+(UNIQUE\s+)?INDEX\s+IF\s+NOT\s+EXISTS"
    if ($usesIndexAlter -and -not $checksIndexes) {
        Write-CheckFail "MySQL 迁移缺少索引幂等保护：$scriptPath"
    } elseif ($usesIndexAlter) {
        Write-CheckOk "MySQL 索引迁移具备幂等保护：$scriptPath"
    }
}

$legacyAuditCleanup = Get-FileText "database/mysql/010_redact_legacy_login_audit_tokens.sql"
if ($legacyAuditCleanup -match "action = 'auth\.login'" -and $legacyAuditCleanup -match "resource_type = 'session'" -and
    $legacyAuditCleanup -match "resource_type = 'user'" -and $legacyAuditCleanup -match "resource_id = actor") {
    Write-CheckOk "历史登录审计 token 清理迁移具备限定条件"
} else {
    Write-CheckFail "历史登录审计 token 清理迁移缺少限定条件"
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
$governanceMigration = Get-FileText "database/mysql/012_seed_account_governance.sql"
if ($governanceMigration -match "requires_password_rotation" -and $governanceMigration -match "induspilot-admin-demo-salt") {
    Write-CheckOk "种子账号治理迁移会标记已发布演示凭据"
} else {
    Write-CheckFail "种子账号治理迁移缺少演示凭据标记逻辑"
}
if ($config -match "(?m)^\s+allow_seed_credentials:\s+true\s*$") {
    Write-CheckWarn "示例配置显式启用演示凭据，仅适用于本地内存演示"
} else {
    Write-CheckFail "示例配置未显式声明演示凭据兼容开关"
}

if ($RequireProductionSecrets) {
    $productionValues = Get-DotEnvValues "deployment/.env"
    if ($null -eq $productionValues) {
        Write-CheckFail "启用了 -RequireProductionSecrets，但缺少 deployment/.env"
    } else {
        foreach ($secret in @("INDUSPILOT_MYSQL_ROOT_PASSWORD", "INDUSPILOT_MYSQL_PASSWORD", "INDUSPILOT_REDIS_PASSWORD", "INDUSPILOT_MONGODB_ROOT_PASSWORD")) {
            Test-ProductionSecretValue $productionValues $secret
        }
        if (-not $productionValues.ContainsKey("INDUSPILOT_SECURITY_PRODUCTION_MODE") -or
            $productionValues["INDUSPILOT_SECURITY_PRODUCTION_MODE"].Trim().ToLowerInvariant() -ne "true") {
            Write-CheckFail "生产环境必须设置 INDUSPILOT_SECURITY_PRODUCTION_MODE=true"
        } else {
            Write-CheckOk "生产环境已启用启动期密钥与种子账号治理"
        }
        if (-not $productionValues.ContainsKey("INDUSPILOT_SECURITY_ALLOW_SEED_CREDENTIALS") -or
            $productionValues["INDUSPILOT_SECURITY_ALLOW_SEED_CREDENTIALS"].Trim().ToLowerInvariant() -ne "false") {
            Write-CheckFail "生产环境必须设置 INDUSPILOT_SECURITY_ALLOW_SEED_CREDENTIALS=false"
        } else {
            Write-CheckOk "生产环境已禁用种子账号兼容登录"
        }
    }
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
