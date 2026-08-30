param(
    [string]$BackendExe = "build/dev-http/backend/induspilot-backend.exe",
    [string]$ConfigPath = "config/backend.example.yaml",
    [string]$EnvPath = "deployment/.env",
    [string]$BaseUrl = "http://127.0.0.1:18081",
    [ValidateSet("memory", "mysql", "mongodb")]
    [string]$AiInteractionStore = "memory",
    [switch]$StartDependencies,
    [switch]$RunDependencySmoke,
    [switch]$ExerciseMongoIndexUpgrade,
    [switch]$ExerciseMongoReadinessFailures,
    [switch]$StopDependencies
)

$ErrorActionPreference = "Stop"
$RepoRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)

function Resolve-RepoPath {
    param([string]$PathValue)
    if ([System.IO.Path]::IsPathRooted($PathValue)) {
        return $PathValue
    }
    return Join-Path $RepoRoot $PathValue
}

function Read-DotEnv {
    param([string]$PathValue)
    $resolved = Resolve-RepoPath $PathValue
    if (-not (Test-Path -LiteralPath $resolved)) {
        throw "缺少依赖环境文件：$resolved。请先从 deployment/.env.example 创建 deployment/.env 并替换密钥。"
    }

    $values = @{}
    foreach ($line in Get-Content -LiteralPath $resolved -Encoding UTF8) {
        $trimmed = $line.Trim()
        if ([string]::IsNullOrWhiteSpace($trimmed) -or $trimmed.StartsWith('#')) {
            continue
        }
        $separator = $trimmed.IndexOf('=')
        if ($separator -le 0) {
            continue
        }
        $key = $trimmed.Substring(0, $separator).Trim()
        $value = $trimmed.Substring($separator + 1).Trim().Trim('"')
        $values[$key] = $value
    }
    return $values
}

function Require-EnvValue {
    param(
        [hashtable]$Values,
        [string]$Name,
        [string]$Fallback = ""
    )
    $value = $Fallback
    if ($Values.ContainsKey($Name) -and -not [string]::IsNullOrWhiteSpace($Values[$Name])) {
        $value = $Values[$Name]
    }
    if ([string]::IsNullOrWhiteSpace($value)) {
        throw "依赖环境缺少必需变量：$Name"
    }
    if ($value -like 'change-me-*') {
        throw "依赖环境仍使用示例密钥：$Name。请先替换 deployment/.env。"
    }
    return $value
}

function Optional-EnvValue {
    param(
        [hashtable]$Values,
        [string]$Name,
        [string]$Fallback
    )
    if ($Values.ContainsKey($Name) -and -not [string]::IsNullOrWhiteSpace($Values[$Name])) {
        return $Values[$Name]
    }
    return $Fallback
}

function Invoke-Compose {
    param([string[]]$Arguments)
    $envFile = Resolve-RepoPath $EnvPath
    & docker compose --env-file $envFile -f (Resolve-RepoPath 'deployment/docker-compose.yml') @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "docker compose 执行失败：$($Arguments -join ' ')"
    }
}

function Invoke-MongoCommand {
    param([string]$Script)
    $envFile = Resolve-RepoPath $EnvPath
    & docker compose --env-file $envFile -f (Resolve-RepoPath 'deployment/docker-compose.yml') exec -T mongodb `
        mongosh --quiet --username $mongoUser --password $mongoPassword --authenticationDatabase admin --eval $Script
    if ($LASTEXITCODE -ne 0) {
        throw "MongoDB command failed while exercising the runtime profile."
    }
}

function Resolve-PowerShellCommand {
    $pwsh = Get-Command pwsh -ErrorAction SilentlyContinue
    if ($null -ne $pwsh) {
        return $pwsh.Source
    }
    $powershell = Get-Command powershell.exe -ErrorAction SilentlyContinue
    if ($null -ne $powershell) {
        return $powershell.Source
    }
    throw "未找到 pwsh 或 powershell.exe，无法运行 HTTP smoke。"
}

$backendExePath = Resolve-RepoPath $BackendExe
$configPathValue = Resolve-RepoPath $ConfigPath
if (-not (Test-Path -LiteralPath $backendExePath)) {
    throw "缺少后端可执行文件：$backendExePath。请先运行 cmake --build --preset dev-http。"
}
if (-not (Test-Path -LiteralPath $configPathValue)) {
    throw "缺少后端配置文件：$configPathValue"
}

$envValues = Read-DotEnv $EnvPath
$powerShellCommand = Resolve-PowerShellCommand
$mysqlPassword = Require-EnvValue $envValues 'INDUSPILOT_MYSQL_PASSWORD'
$redisPassword = Require-EnvValue $envValues 'INDUSPILOT_REDIS_PASSWORD'
$mongoUser = Require-EnvValue $envValues 'INDUSPILOT_MONGODB_ROOT_USER' 'induspilot'
$mongoPassword = Require-EnvValue $envValues 'INDUSPILOT_MONGODB_ROOT_PASSWORD'

$mysqlHost = Optional-EnvValue $envValues 'INDUSPILOT_MYSQL_BIND' '127.0.0.1'
$mysqlPort = Optional-EnvValue $envValues 'INDUSPILOT_MYSQL_PORT' '3306'
$mysqlDatabase = Optional-EnvValue $envValues 'INDUSPILOT_MYSQL_DATABASE' 'induspilot'
$mysqlUser = Optional-EnvValue $envValues 'INDUSPILOT_MYSQL_USER' 'induspilot'
$redisHost = Optional-EnvValue $envValues 'INDUSPILOT_REDIS_BIND' '127.0.0.1'
$redisPort = Optional-EnvValue $envValues 'INDUSPILOT_REDIS_PORT' '6379'
$mongoHost = Optional-EnvValue $envValues 'INDUSPILOT_MONGODB_BIND' '127.0.0.1'
$mongoPort = Optional-EnvValue $envValues 'INDUSPILOT_MONGODB_PORT' '27017'

$mysqlUri = "host=$mysqlHost port=$mysqlPort dbname=$mysqlDatabase user=$mysqlUser password=$mysqlPassword"
$redisUri = "tcp://:$redisPassword@$redisHost`:$redisPort/0"
$mongoUri = "mongodb://$mongoUser`:$mongoPassword@$mongoHost`:$mongoPort/admin"

if (($StartDependencies -or $RunDependencySmoke -or $ExerciseMongoIndexUpgrade -or $ExerciseMongoReadinessFailures) -and -not (Get-Command docker -ErrorAction SilentlyContinue)) {
    throw "未找到 docker 命令，无法启动或验证真实依赖。"
}
if (($ExerciseMongoIndexUpgrade -or $ExerciseMongoReadinessFailures) -and $AiInteractionStore -ne 'mongodb') {
    throw "MongoDB runtime exercises require AiInteractionStore=mongodb."
}

try {
    if ($StartDependencies) {
        Write-Host "[runtime-smoke] 启动 compose 依赖"
        Invoke-Compose @('up', '-d', '--wait')
    }

    if ($RunDependencySmoke) {
        $bash = Get-Command bash -ErrorAction SilentlyContinue
        if ($null -eq $bash -and (Test-Path -LiteralPath 'C:\Program Files\Git\bin\bash.exe')) {
            $bashPath = 'C:\Program Files\Git\bin\bash.exe'
        } elseif ($null -ne $bash) {
            $bashPath = $bash.Source
        } else {
            throw "未找到 bash，无法运行 backend/tests/dependency_services_smoke.sh。"
        }
        Write-Host "[runtime-smoke] 运行依赖 CRUD smoke"
        & $bashPath (Resolve-RepoPath 'backend/tests/dependency_services_smoke.sh')
        if ($LASTEXITCODE -ne 0) {
            throw "dependency_services_smoke.sh 执行失败"
        }
    }

    if ($ExerciseMongoIndexUpgrade) {
        Write-Host "[runtime-smoke] 模拟旧 MongoDB 集合缺少 interactionCode 唯一索引"
        Invoke-MongoCommand 'const collection = db.getSiblingDB("induspilot").ai_interactions; try { collection.dropIndex("interactionCode_1"); } catch (error) { if (error.codeName !== "IndexNotFound") { throw error; } }'
    }

    Write-Host "[runtime-smoke] 运行 HTTP smoke：repository_store=mysql session_store=redis"
    & $powerShellCommand -NoProfile -ExecutionPolicy Bypass -File (Resolve-RepoPath 'backend/tests/http_integration_smoke.ps1') `
        -BackendExe $backendExePath `
        -ConfigPath $configPathValue `
        -BaseUrl $BaseUrl `
        -RepositoryStore mysql `
        -AiInteractionStore $AiInteractionStore `
        -SessionStore redis `
        -MySqlUri $mysqlUri `
        -MySqlDatabase $mysqlDatabase `
        -RedisUri $redisUri `
        -MongoDbUri $mongoUri
    if ($LASTEXITCODE -ne 0) {
        throw "HTTP runtime profile smoke 执行失败"
    }

    if ($ExerciseMongoIndexUpgrade) {
        Invoke-MongoCommand 'const index = db.getSiblingDB("induspilot").ai_interactions.getIndexes().find(item => item.name === "interactionCode_1"); if (!index || index.unique !== true) { throw new Error("backend did not reconcile the unique interactionCode index"); } print("mongodb_index_upgrade_smoke_passed");'
    }

    if ($ExerciseMongoReadinessFailures) {
        $wrongMongoPassword = $mongoPassword + '-invalid'
        $wrongMongoUri = "mongodb://$mongoUser`:$wrongMongoPassword@$mongoHost`:$mongoPort/admin"
        Write-Host "[runtime-smoke] 验证 MongoDB 错误凭据 readiness 失败和诊断脱敏"
        & $powerShellCommand -NoProfile -ExecutionPolicy Bypass -File (Resolve-RepoPath 'backend/tests/http_integration_smoke.ps1') `
            -BackendExe $backendExePath `
            -ConfigPath $configPathValue `
            -BaseUrl $BaseUrl `
            -RepositoryStore mysql `
            -AiInteractionStore mongodb `
            -SessionStore redis `
            -MySqlUri $mysqlUri `
            -MySqlDatabase $mysqlDatabase `
            -RedisUri $redisUri `
            -MongoDbUri $wrongMongoUri `
            -ReadinessOnly `
            -ExpectNotReady `
            -ExpectedUnavailableDependency mongodb `
            -ExpectMongoReadinessFailure
        if ($LASTEXITCODE -ne 0) {
            throw "MongoDB readiness failure smoke 执行失败"
        }

        Write-Host "[runtime-smoke] 验证 MongoDB 凭据恢复后 readiness 成功"
        & $powerShellCommand -NoProfile -ExecutionPolicy Bypass -File (Resolve-RepoPath 'backend/tests/http_integration_smoke.ps1') `
            -BackendExe $backendExePath `
            -ConfigPath $configPathValue `
            -BaseUrl $BaseUrl `
            -RepositoryStore mysql `
            -AiInteractionStore mongodb `
            -SessionStore redis `
            -MySqlUri $mysqlUri `
            -MySqlDatabase $mysqlDatabase `
            -RedisUri $redisUri `
            -MongoDbUri $mongoUri `
            -ReadinessOnly
        if ($LASTEXITCODE -ne 0) {
            throw "MongoDB readiness recovery smoke 执行失败"
        }

        $restrictedMongoUser = $mongoUser + '-readonly'
        $restrictedMongoPassword = $mongoPassword + '-readonly'
        $restrictedUserLiteral = $restrictedMongoUser | ConvertTo-Json -Compress
        $restrictedPasswordLiteral = $restrictedMongoPassword | ConvertTo-Json -Compress
        Write-Host "[runtime-smoke] 验证 MongoDB 业务库权限不足时 fail closed"
        Invoke-MongoCommand "const database = db.getSiblingDB('induspilot'); if (database.getUser($restrictedUserLiteral)) { database.dropUser($restrictedUserLiteral); } database.createUser({user: $restrictedUserLiteral, pwd: $restrictedPasswordLiteral, roles: [{role: 'read', db: 'induspilot'}]});"
        $restrictedMongoUri = "mongodb://$restrictedMongoUser`:$restrictedMongoPassword@$mongoHost`:$mongoPort/induspilot"
        & $powerShellCommand -NoProfile -ExecutionPolicy Bypass -File (Resolve-RepoPath 'backend/tests/http_integration_smoke.ps1') `
            -BackendExe $backendExePath `
            -ConfigPath $configPathValue `
            -BaseUrl $BaseUrl `
            -RepositoryStore mysql `
            -AiInteractionStore mongodb `
            -SessionStore redis `
            -MySqlUri $mysqlUri `
            -MySqlDatabase $mysqlDatabase `
            -RedisUri $redisUri `
            -MongoDbUri $restrictedMongoUri `
            -ExpectStartupFailure `
            -ExpectMongoCredentialRedaction
        if ($LASTEXITCODE -ne 0) {
            throw "MongoDB permission failure smoke 执行失败"
        }
        Invoke-MongoCommand "db.getSiblingDB('induspilot').dropUser($restrictedUserLiteral);"
    }

    Write-Host "[runtime-smoke] HTTP runtime profile smoke passed"
} finally {
    if ($StopDependencies) {
        Write-Host "[runtime-smoke] 清理 compose 依赖"
        Invoke-Compose @('down', '-v')
    }
}
