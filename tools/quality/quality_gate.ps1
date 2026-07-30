param(
    [switch]$RequireClangTools
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot '..\..')
$failures = New-Object System.Collections.Generic.List[string]

function Add-Failure([string]$message) {
    $failures.Add($message) | Out-Null
}

function Require-File([string]$relativePath) {
    $path = Join-Path $repoRoot $relativePath
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        Add-Failure "缺少必需文件：$relativePath"
        return $null
    }
    return $path
}

function Require-Directory([string]$relativePath) {
    $path = Join-Path $repoRoot $relativePath
    if (-not (Test-Path -LiteralPath $path -PathType Container)) {
        Add-Failure "缺少必需目录：$relativePath"
        return $null
    }
    return $path
}

function Read-Text([string]$relativePath) {
    $path = Require-File $relativePath
    if ($null -eq $path) { return '' }
    return [System.IO.File]::ReadAllText($path, [System.Text.Encoding]::UTF8)
}

function Assert-Contains([string]$name, [string]$content, [string]$needle) {
    if (-not $content.Contains($needle)) {
        Add-Failure "$name 缺少约束：$needle"
    }
}

Write-Host '== IndusPilot 质量门禁 =='

$clangFormatPath = Require-File '.clang-format'
$clangTidyPath = Require-File '.clang-tidy'
$presetsPath = Require-File 'CMakePresets.json'
$workflowPath = Require-File '.github/workflows/ci.yml'
Require-Directory 'openspec/specs' | Out-Null
Require-Directory 'backend' | Out-Null
Require-Directory 'client' | Out-Null
Require-Directory 'deployment' | Out-Null

if ($clangFormatPath) {
    $clangFormat = Read-Text '.clang-format'
    Assert-Contains '.clang-format' $clangFormat 'BasedOnStyle: LLVM'
    Assert-Contains '.clang-format' $clangFormat 'ColumnLimit: 120'
    Assert-Contains '.clang-format' $clangFormat 'Standard: c++17'
}

if ($clangTidyPath) {
    $clangTidy = Read-Text '.clang-tidy'
    Assert-Contains '.clang-tidy' $clangTidy 'bugprone-*'
    Assert-Contains '.clang-tidy' $clangTidy 'clang-analyzer-*'
    Assert-Contains '.clang-tidy' $clangTidy 'WarningsAsErrors'
}

if ($presetsPath) {
    $presetText = Read-Text 'CMakePresets.json'
    if ($presetText -match 'C:[/\\]Users[/\\][^/\\]+[/\\]vcpkg') {
        Add-Failure 'CMakePresets.json 不允许写死本机用户目录中的 vcpkg 路径'
    }

    $presets = $presetText | ConvertFrom-Json
    foreach ($presetName in @('dev', 'dev-redis', 'dev-http')) {
        if (-not ($presets.configurePresets | Where-Object { $_.name -eq $presetName })) {
            Add-Failure "CMake configure preset 缺少：$presetName"
        }
        if (-not ($presets.buildPresets | Where-Object { $_.name -eq $presetName })) {
            Add-Failure "CMake build preset 缺少：$presetName"
        }
        if (-not ($presets.testPresets | Where-Object { $_.name -eq $presetName })) {
            Add-Failure "CMake test preset 缺少：$presetName"
        }
    }
}

if ($workflowPath) {
    $workflow = Read-Text '.github/workflows/ci.yml'
    foreach ($required in @('quality-gates:', 'backend-foundation:', 'configuration-preflight:', 'dependency-services:', 'openspec:')) {
        Assert-Contains '.github/workflows/ci.yml' $workflow $required
    }
    Assert-Contains '.github/workflows/ci.yml' $workflow 'tools/quality/quality_gate.ps1 -RequireClangTools'
    Assert-Contains '.github/workflows/ci.yml' $workflow 'openspec validate --specs --strict'
}

$clangFormatCommand = Get-Command clang-format -ErrorAction SilentlyContinue
$clangTidyCommand = Get-Command clang-tidy -ErrorAction SilentlyContinue
if ($RequireClangTools) {
    if ($null -eq $clangFormatCommand) { Add-Failure 'CI 质量门禁要求 clang-format 可执行文件可用' }
    if ($null -eq $clangTidyCommand) { Add-Failure 'CI 质量门禁要求 clang-tidy 可执行文件可用' }
}

if ($clangFormatCommand) {
    Write-Host "clang-format: $(& clang-format --version)"
} else {
    Write-Host 'clang-format: 本地未安装，已跳过工具版本检查'
}

if ($clangTidyCommand) {
    Write-Host "clang-tidy: $(& clang-tidy --version | Select-Object -First 1)"
} else {
    Write-Host 'clang-tidy: 本地未安装，已跳过工具版本检查'
}

if ($failures.Count -gt 0) {
    Write-Host '质量门禁失败：'
    foreach ($failure in $failures) {
        Write-Host " - $failure"
    }
    exit 1
}

Write-Host '质量门禁通过。'