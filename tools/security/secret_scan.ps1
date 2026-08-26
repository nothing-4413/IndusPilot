param(
    [switch]$FailOnMissingGit
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot '..\..')
$failures = New-Object System.Collections.Generic.List[string]

function Add-Failure {
    param([string]$Message)
    $failures.Add($Message) | Out-Null
}

function Test-TextFile {
    param([string]$RelativePath)
    $skipExtensions = @('.png', '.jpg', '.jpeg', '.gif', '.ico', '.pdf', '.zip', '.7z', '.dll', '.exe', '.pdb')
    $extension = [System.IO.Path]::GetExtension($RelativePath).ToLowerInvariant()
    return -not ($skipExtensions -contains $extension)
}

Write-Host '== IndusPilot 密钥扫描 =='

if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
    $message = '未找到 git 命令，无法按 tracked files 执行密钥扫描'
    if ($FailOnMissingGit) {
        Add-Failure $message
    } else {
        Write-Host "[WARN] $message"
    }
} else {
    $trackedFiles = & git -C $repoRoot ls-files
    if ($LASTEXITCODE -ne 0) {
        Add-Failure 'git ls-files 执行失败'
        $trackedFiles = @()
    }

    foreach ($relativePath in $trackedFiles) {
        $normalized = $relativePath -replace '\\', '/'
        $fileName = [System.IO.Path]::GetFileName($normalized)
        if ($fileName -eq '.env') {
            Add-Failure "禁止提交真实环境文件：$normalized"
            continue
        }
        if (-not (Test-TextFile $normalized)) {
            continue
        }

        $fullPath = Join-Path $repoRoot $normalized
        if (-not (Test-Path -LiteralPath $fullPath -PathType Leaf)) {
            continue
        }

        $lineNumber = 0
        foreach ($line in Get-Content -LiteralPath $fullPath -Encoding UTF8) {
            $lineNumber += 1
            if ($line -match '-----BEGIN (RSA |DSA |EC |OPENSSH |)PRIVATE KEY-----') {
                Add-Failure "${normalized}:$lineNumber 疑似提交私钥内容"
            }
            if ($line -match '\bgh[pousr]_[A-Za-z0-9_]{30,}\b' -or $line -match '\bgithub_pat_[A-Za-z0-9_]{30,}\b') {
                Add-Failure "${normalized}:$lineNumber 疑似 GitHub token"
            }
            if ($line -match '\bsk-(proj-)?[A-Za-z0-9_-]{24,}\b') {
                Add-Failure "${normalized}:$lineNumber 疑似 OpenAI/API secret key"
            }
            if ($line -match '\b(AKIA|ASIA)[A-Z0-9]{16}\b') {
                Add-Failure "${normalized}:$lineNumber 疑似 AWS access key"
            }
        }
    }
}

if ($failures.Count -gt 0) {
    Write-Host '密钥扫描失败：'
    foreach ($failure in $failures) {
        Write-Host " - $failure"
    }
    exit 1
}

Write-Host '密钥扫描通过。'