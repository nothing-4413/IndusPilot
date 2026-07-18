param(
    [Parameter(Mandatory = $true)]
    [string]$BackendExe,

    [Parameter(Mandatory = $true)]
    [string]$ConfigPath,

    [string]$BaseUrl = "http://127.0.0.1:18081",
    [string]$SessionStore = "memory"
)

$ErrorActionPreference = "Stop"

function Assert-True {
    param(
        [bool]$Condition,
        [string]$Message
    )
    if (-not $Condition) {
        throw $Message
    }
}

function Invoke-ExpectStatus {
    param(
        [string]$Uri,
        [string]$Method,
        [int]$Status,
        [hashtable]$Headers = @{},
        [string]$Body = $null
    )

    try {
        if (-not $PSBoundParameters.ContainsKey("Body") -or $Body -eq $null) {
            Invoke-RestMethod -Uri $Uri -Method $Method -Headers $Headers -TimeoutSec 10 | Out-Null
        } else {
            Invoke-RestMethod -Uri $Uri -Method $Method -Headers $Headers -ContentType "application/json" -Body $Body -TimeoutSec 10 | Out-Null
        }
        throw "Expected HTTP $Status from $Method $Uri"
    } catch {
        $response = $_.Exception.Response
        if ($response -eq $null) {
            throw
        }
        $actual = [int]$response.StatusCode
        if ($actual -ne $Status) {
            throw "Expected HTTP $Status from $Method $Uri, got $actual"
        }
    }
}

$oldPort = $env:INDUSPILOT_SERVER_PORT
$oldSessionStore = $env:INDUSPILOT_REDIS_SESSION_STORE
$env:INDUSPILOT_SERVER_PORT = "18081"
$env:INDUSPILOT_REDIS_SESSION_STORE = $SessionStore

$stdout = [System.IO.Path]::GetTempFileName()
$stderr = [System.IO.Path]::GetTempFileName()
$proc = Start-Process -FilePath $BackendExe -ArgumentList $ConfigPath -WorkingDirectory (Split-Path -Parent (Split-Path -Parent $BackendExe)) -WindowStyle Hidden -RedirectStandardOutput $stdout -RedirectStandardError $stderr -PassThru

try {
    Start-Sleep -Seconds 3

    $health = Invoke-RestMethod -Uri "$BaseUrl/health" -Method Get -Headers @{ "X-Trace-Id" = "trace-it-health" } -TimeoutSec 10
    Assert-True ($health.service -eq "induspilot-backend") "Health check did not return service name."
    Assert-True ($null -ne $health.dependencies.mysql) "Health check did not include MySQL dependency."
    Assert-True ($null -ne $health.dependencies.redis) "Health check did not include Redis dependency."
    Assert-True ($null -ne $health.dependencies.mongodb) "Health check did not include MongoDB dependency."
    Assert-True ($null -ne $health.dependencies.ai) "Health check did not include AI dependency."

    Invoke-ExpectStatus -Uri "$BaseUrl/api/v1/assets" -Method Get -Status 401

    $operatorLogin = Invoke-RestMethod -Uri "$BaseUrl/api/v1/auth/login" -Method Post -ContentType "application/json" -Body '{"username":"operator","password":"operator123"}' -TimeoutSec 10
    Assert-True $operatorLogin.success "Operator login failed."
    $operatorToken = $operatorLogin.data.token
    $operatorHeaders = @{ Authorization = "Bearer $operatorToken"; "X-Trace-Id" = "trace-it-operator" }

    $session = Invoke-RestMethod -Uri "$BaseUrl/api/v1/auth/session" -Method Get -Headers $operatorHeaders -TimeoutSec 10
    Assert-True $session.success "Session validation failed."

    $assets = Invoke-RestMethod -Uri "$BaseUrl/api/v1/assets" -Method Get -Headers $operatorHeaders -TimeoutSec 10
    Assert-True $assets.success "Protected asset read failed."
    Invoke-ExpectStatus -Uri "$BaseUrl/api/v1/assets" -Method Post -Status 403 -Headers $operatorHeaders -Body '{"id":"asset-it-denied","name":"denied"}'

    $adminLogin = Invoke-RestMethod -Uri "$BaseUrl/api/v1/auth/login" -Method Post -ContentType "application/json" -Body '{"username":"admin","password":"admin123"}' -TimeoutSec 10
    Assert-True $adminLogin.success "Admin login failed."
    $adminToken = $adminLogin.data.token
    $adminHeaders = @{ Authorization = "Bearer $adminToken"; "X-Trace-Id" = "trace-it-admin" }

    Invoke-ExpectStatus -Uri "$BaseUrl/api/v1/assets" -Method Post -Status 400 -Headers $adminHeaders -Body '{"name":"missing id"}'
    Invoke-ExpectStatus -Uri "$BaseUrl/api/v1/assets/not-exist" -Method Get -Status 404 -Headers $adminHeaders

    $created = Invoke-RestMethod -Uri "$BaseUrl/api/v1/assets" -Method Post -Headers $adminHeaders -ContentType "application/json" -Body '{"id":"asset-it-001","name":"integration asset"}' -TimeoutSec 10
    Assert-True $created.success "Admin asset creation failed."
} finally {
    if ($proc -and -not $proc.HasExited) {
        Stop-Process -Id $proc.Id -Force
        Wait-Process -Id $proc.Id -Timeout 10 -ErrorAction SilentlyContinue
    }
    $env:INDUSPILOT_SERVER_PORT = $oldPort
    $env:INDUSPILOT_REDIS_SESSION_STORE = $oldSessionStore
    Remove-Item -LiteralPath $stdout, $stderr -Force -ErrorAction SilentlyContinue
}