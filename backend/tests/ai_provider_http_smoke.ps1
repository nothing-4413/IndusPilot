param(
    [string]$BackendExe = "build/dev-http/backend/induspilot-backend.exe",
    [string]$ConfigPath = "config/backend.example.yaml"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)

function Resolve-RepoPath {
    param([string]$PathValue)
    if ([System.IO.Path]::IsPathRooted($PathValue)) {
        return $PathValue
    }
    return Join-Path $repoRoot $PathValue
}

function Assert-True {
    param([bool]$Condition, [string]$Message)
    if (-not $Condition) {
        throw $Message
    }
}

function Get-FreePort {
    $probe = New-Object System.Net.Sockets.TcpListener -ArgumentList ([System.Net.IPAddress]::Loopback), 0
    try {
        $probe.Start()
        return ([System.Net.IPEndPoint]$probe.LocalEndpoint).Port
    } finally {
        $probe.Stop()
    }
}

function Wait-ForFile {
    param([string]$PathValue, [System.Diagnostics.Process]$Process)
    for ($attempt = 0; $attempt -lt 40; $attempt++) {
        if (Test-Path -LiteralPath $PathValue) {
            return
        }
        if ($Process.HasExited) {
            throw "fake provider exited before becoming ready"
        }
        Start-Sleep -Milliseconds 50
    }
    throw "fake provider did not become ready"
}

function Wait-ForHttp {
    param([string]$Uri, [System.Diagnostics.Process]$Process)
    for ($attempt = 0; $attempt -lt 40; $attempt++) {
        if ($Process.HasExited) {
            throw "backend exited before becoming ready"
        }
        try {
            Invoke-RestMethod -Uri $Uri -Method Get -TimeoutSec 1 | Out-Null
            return
        } catch {
            Start-Sleep -Milliseconds 100
        }
    }
    throw "backend did not become ready"
}

function Invoke-Scenario {
    param(
        [string]$Name,
        [string]$Mode,
        [bool]$ExpectAvailable,
        [int]$ProviderDelayMs = 500,
        [int]$MaxRetries = 0,
        [int]$MaxResponseBytes = 1048576
    )

    $backendPort = Get-FreePort
    $providerPort = Get-FreePort
    $providerLog = [System.IO.Path]::GetTempFileName()
    $providerReady = [System.IO.Path]::GetTempFileName()
    $providerStdout = [System.IO.Path]::GetTempFileName()
    $providerStderr = [System.IO.Path]::GetTempFileName()
    $backendStdout = [System.IO.Path]::GetTempFileName()
    $backendStderr = [System.IO.Path]::GetTempFileName()
    Remove-Item -LiteralPath $providerLog, $providerReady, $providerStdout, $providerStderr, $backendStdout, $backendStderr -Force
    $providerProcess = $null
    $backendProcess = $null

    try {
        $fakeProvider = Resolve-RepoPath "backend/tests/fake_ai_provider.ps1"
        $providerArguments = @(
            "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", $fakeProvider,
            "-Port", [string]$providerPort, "-RequestLog", $providerLog,
            "-ReadyFile", $providerReady, "-Mode", $Mode, "-DelayMs", [string]$ProviderDelayMs
        )
        $providerProcess = Start-Process -FilePath "powershell.exe" -ArgumentList $providerArguments `
            -WorkingDirectory $repoRoot -WindowStyle Hidden -RedirectStandardOutput $providerStdout `
            -RedirectStandardError $providerStderr -PassThru
        Wait-ForFile $providerReady $providerProcess

        $env:INDUSPILOT_SERVER_PORT = [string]$backendPort
        $env:INDUSPILOT_REPOSITORY_STORE = "memory"
        $env:INDUSPILOT_REDIS_SESSION_STORE = "memory"
        $env:INDUSPILOT_AI_ENABLED = "true"
        $env:INDUSPILOT_AI_PROVIDER = "http"
        $env:INDUSPILOT_AI_ENDPOINT = "http://127.0.0.1:$providerPort/v1/complete"
        $env:INDUSPILOT_AI_API_KEY = "test-ai-key"
        $env:INDUSPILOT_AI_AUTH_HEADER = "X-Test-AI-Key"
        $env:INDUSPILOT_AI_AUTH_SCHEME = "Token"
        $env:INDUSPILOT_AI_TIMEOUT_MS = if ($Mode -eq "timeout") { "100" } else { "2000" }
        $env:INDUSPILOT_AI_MAX_RETRIES = [string]$MaxRetries
        $env:INDUSPILOT_AI_MAX_RESPONSE_BYTES = [string]$MaxResponseBytes
        $env:INDUSPILOT_AI_MAX_CONTEXT_ITEMS = "2"
        $env:INDUSPILOT_AI_REQUIRE_STRUCTURED_RESPONSE = "true"
        $env:INDUSPILOT_AI_STORE_INTERACTION_RECORDS = "true"

        $resolvedBackend = Resolve-RepoPath $BackendExe
        $resolvedConfig = Resolve-RepoPath $ConfigPath
        $backendProcess = Start-Process -FilePath $resolvedBackend -ArgumentList $resolvedConfig `
            -WorkingDirectory $repoRoot -WindowStyle Hidden -RedirectStandardOutput $backendStdout `
            -RedirectStandardError $backendStderr -PassThru
        $baseUrl = "http://127.0.0.1:$backendPort"
        Wait-ForHttp "$baseUrl/health/live" $backendProcess

        $login = Invoke-RestMethod -Uri "$baseUrl/api/v1/auth/login" -Method Post -ContentType "application/json" `
            -Body '{"username":"operator","password":"operator123"}' -TimeoutSec 10
        Assert-True $login.success "${Name}: operator login failed"
        $headers = @{ Authorization = "Bearer $($login.data.token)" }
        $diagnosisBody = '{"relatedType":"alert","relatedId":"ai-provider-smoke","prompt":"diagnose critical temperature token=prompt-secret","context":{"assetId":"asset-smoke","severity":"critical","operatorDescription":"password=operator-secret","contextItems":["token=client-secret","second","third"]}}'
        $diagnosis = Invoke-RestMethod -Uri "$baseUrl/api/v1/ai/diagnose" -Method Post -Headers $headers `
            -ContentType "application/json" -Body $diagnosisBody -TimeoutSec 10
        Assert-True $diagnosis.success "${Name}: diagnosis request failed"
        Assert-True ([bool]$diagnosis.data.available -eq $ExpectAvailable) "${Name}: provider availability mismatch"
        Assert-True ($diagnosis.data.provider -eq "http") "${Name}: provider name mismatch"
        Assert-True ($diagnosis.data.riskLevel -eq "critical") "${Name}: local risk orchestration was not preserved"
        Assert-True ([bool]$diagnosis.data.requiresHumanReview) "${Name}: human review flag was not preserved"

        $interaction = Invoke-RestMethod -Uri "$baseUrl/api/v1/ai/interactions?relatedId=ai-provider-smoke" -Method Get -Headers $headers -TimeoutSec 10
        Assert-True (@($interaction.data).Count -eq 1) "${Name}: AI interaction audit was not written"
        $interactionItem = @($interaction.data)[0]
        Assert-True (-not ([string]$interactionItem.input).Contains("prompt-secret")) "${Name}: prompt secret reached AI audit"
        Assert-True (-not ([string]$interactionItem.input).Contains("operator-secret")) "${Name}: context secret reached AI audit"

        for ($attempt = 0; $attempt -lt 40; $attempt++) {
            if (Test-Path -LiteralPath $providerLog) {
                $logLines = @(Get-Content -LiteralPath $providerLog | Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
                if ($logLines.Count -ge 1) {
                    break
                }
            }
            Start-Sleep -Milliseconds 50
        }
        Assert-True (Test-Path -LiteralPath $providerLog) "${Name}: provider request log was not created"
        $records = @(Get-Content -LiteralPath $providerLog | Where-Object { -not [string]::IsNullOrWhiteSpace($_) } | ForEach-Object { $_ | ConvertFrom-Json })
        Assert-True ($records.Count -ge 1) "${Name}: provider request log was empty"
        $record = $records[0]
        Assert-True ($record.method -eq "POST") "${Name}: provider method mismatch"
        Assert-True ($record.path -eq "/v1/complete") "${Name}: provider path mismatch"
        Assert-True ($record.headers.'X-IndusPilot-Ai-Operation' -eq "diagnose") "${Name}: provider operation header mismatch"
        Assert-True ($record.headers.'X-Test-AI-Key' -eq "Token test-ai-key") "${Name}: provider auth header mismatch"
        $providerBody = $record.body | ConvertFrom-Json
        Assert-True ($providerBody.operation -eq "diagnose") "${Name}: provider operation body mismatch"
        Assert-True ($providerBody.prompt -like "*critical temperature*") "${Name}: provider prompt mismatch"
        Assert-True (@($providerBody.contextItems).Count -eq 2) "${Name}: provider context bound was not enforced"
        Assert-True (-not ([string]$providerBody.prompt).Contains("prompt-secret")) "${Name}: prompt secret reached provider"
        Assert-True (-not ([string]$providerBody.contextItems[0]).Contains("client-secret")) "${Name}: context secret reached provider"

        if ($Mode -eq "success") {
            Assert-True ($diagnosis.data.rawProviderOutput -eq "fake provider response") "${Name}: provider content was not preserved"
        } elseif ($Mode -eq "retry-success") {
            Assert-True ($diagnosis.data.rawProviderOutput -eq "retry provider response") "${Name}: retry response was not preserved"
        } else {
            Assert-True (-not [string]::IsNullOrWhiteSpace([string]$diagnosis.data.rawProviderOutput)) "${Name}: fallback reason was not returned"
            Assert-True ([string]$diagnosis.data.rawProviderOutput -like "*HTTP provider*") "${Name}: provider failure was not surfaced"
        }
        Write-Host "[ai-provider-smoke] $Name passed"
    } finally {
        if ($backendProcess -and -not $backendProcess.HasExited) {
            Stop-Process -Id $backendProcess.Id -Force
            Wait-Process -Id $backendProcess.Id -Timeout 10 -ErrorAction SilentlyContinue
        }
        if ($providerProcess -and -not $providerProcess.HasExited) {
            Stop-Process -Id $providerProcess.Id -Force
            Wait-Process -Id $providerProcess.Id -Timeout 10 -ErrorAction SilentlyContinue
        }
        Remove-Item -LiteralPath $providerLog, $providerReady, $providerStdout, $providerStderr, $backendStdout, $backendStderr -Force -ErrorAction SilentlyContinue
    }
}

$oldEnvironment = @{}
foreach ($name in @(
    "INDUSPILOT_SERVER_PORT", "INDUSPILOT_REPOSITORY_STORE", "INDUSPILOT_REDIS_SESSION_STORE",
    "INDUSPILOT_AI_ENABLED", "INDUSPILOT_AI_PROVIDER", "INDUSPILOT_AI_ENDPOINT", "INDUSPILOT_AI_API_KEY",
    "INDUSPILOT_AI_AUTH_HEADER", "INDUSPILOT_AI_AUTH_SCHEME", "INDUSPILOT_AI_TIMEOUT_MS",
    "INDUSPILOT_AI_MAX_RETRIES", "INDUSPILOT_AI_MAX_RESPONSE_BYTES",
    "INDUSPILOT_AI_MAX_CONTEXT_ITEMS", "INDUSPILOT_AI_REQUIRE_STRUCTURED_RESPONSE",
    "INDUSPILOT_AI_STORE_INTERACTION_RECORDS"
)) {
    $oldEnvironment[$name] = [Environment]::GetEnvironmentVariable($name)
}

try {
    Invoke-Scenario "success" "success" $true
    Invoke-Scenario "failure" "failure" $false
    Invoke-Scenario "non-json" "non-json" $false
    Invoke-Scenario "retry-success" "retry-success" $true 500 1
    Invoke-Scenario "oversized" "oversized" $false 500 0 128
    Invoke-Scenario "timeout" "timeout" $false 500 1
    Write-Host "[ai-provider-smoke] all scenarios passed"
} finally {
    foreach ($entry in $oldEnvironment.GetEnumerator()) {
        [Environment]::SetEnvironmentVariable($entry.Key, $entry.Value)
    }
}
