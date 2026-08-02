param(
    [string]$BackendExe = "build/dev-http/backend/induspilot-backend.exe",
    [string]$ConfigPath = "config/backend.example.yaml",
    [string]$EnvPath = "deployment/.env",
    [string]$BaseUrl = "http://127.0.0.1:18081",
    [switch]$StartDependencies,
    [switch]$RunDependencySmoke,
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

$backendExePath = Resolve-RepoPath $BackendExe
$configPathValue = Resolve-RepoPath $ConfigPath
if (-not (Test-Path -LiteralPath $backendExePath)) {
    throw "缺少后端可执行文件：$backendExePath。请先运行 cmake --build --preset dev-http。"
}
if (-not (Test-Path -LiteralPath $configPathValue)) {
    throw "缺少后端配置文件：$configPathValue"
}

$envValues = Read-DotEnv $EnvPath
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

if (($StartDependencies -or $RunDependencySmoke) -and -not (Get-Command docker -ErrorAction SilentlyContinue)) {
    throw "未找到 docker 命令，无法启动或验证真实依赖。"
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

    Write-Host "[runtime-smoke] 运行 HTTP smoke：repository_store=mysql session_store=redis"
    & powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Resolve-RepoPath 'backend/tests/http_integration_smoke.ps1') `
        -BackendExe $backendExePath `
        -ConfigPath $configPathValue `
        -BaseUrl $BaseUrl `
        -RepositoryStore mysql `
        -SessionStore redis `
        -MySqlUri $mysqlUri `
        -RedisUri $redisUri `
        -MongoDbUri $mongoUri
    if ($LASTEXITCODE -ne 0) {
        throw "HTTP runtime profile smoke 执行失败"
    }

    Write-Host "[runtime-smoke] HTTP runtime profile smoke passed"
} finally {
    if ($StopDependencies) {
        Write-Host "[runtime-smoke] 清理 compose 依赖"
        Invoke-Compose @('down', '-v')
    }
}