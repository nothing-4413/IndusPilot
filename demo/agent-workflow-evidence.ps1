param(
    [string]$OutputDir = "build\agent-workflow-evidence",
    [string]$ClosedLoopOutputDir = "build\demo-closed-loop-output",
    [int]$RecentCommitCount = 12
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$outputPath = Join-Path $repoRoot $OutputDir
New-Item -ItemType Directory -Force -Path $outputPath | Out-Null

function Invoke-GitLines([string[]]$Arguments) {
    $lines = & git -C $repoRoot @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "git $($Arguments -join ' ') 执行失败"
    }
    return @($lines)
}

function Get-RelativePath([string]$Path) {
    $rootText = $repoRoot.Path.TrimEnd('\')
    $fullPath = (Resolve-Path -LiteralPath $Path).Path
    if ($fullPath.StartsWith($rootText, [System.StringComparison]::OrdinalIgnoreCase)) {
        $relative = $fullPath.Substring($rootText.Length).TrimStart('\')
        if ([string]::IsNullOrWhiteSpace($relative)) { return '.' }
        return $relative.Replace('\', '/')
    }
    return $fullPath.Replace('\', '/')
}

$commits = Invoke-GitLines @('log', "--max-count=$RecentCommitCount", '--pretty=format:%h|%s|%cI') | ForEach-Object {
    $parts = $_ -split '\|', 3
    [pscustomobject]@{
        sha = $parts[0]
        message = $parts[1]
        committedAt = $parts[2]
    }
}

$archiveRoot = Join-Path $repoRoot 'openspec\changes\archive'
$archives = @()
if (Test-Path -LiteralPath $archiveRoot) {
    $archives = Get-ChildItem -LiteralPath $archiveRoot -Directory |
        Sort-Object Name -Descending |
        Select-Object -First 20 |
        ForEach-Object {
            [pscustomobject]@{
                name = $_.Name
                proposal = (Test-Path -LiteralPath (Join-Path $_.FullName 'proposal.md'))
                tasks = (Test-Path -LiteralPath (Join-Path $_.FullName 'tasks.md'))
            }
        }
}

$workflowPath = Join-Path $repoRoot '.github\workflows\ci.yml'
$workflowText = if (Test-Path -LiteralPath $workflowPath) { [System.IO.File]::ReadAllText($workflowPath, [System.Text.Encoding]::UTF8) } else { '' }
$workflowJobs = [regex]::Matches($workflowText, '(?m)^  ([A-Za-z0-9_-]+):\s*$') | ForEach-Object { $_.Groups[1].Value }

$closedLoopSummaryPath = Join-Path $repoRoot (Join-Path $ClosedLoopOutputDir '00-summary.json')
$closedLoopSummary = $null
if (Test-Path -LiteralPath $closedLoopSummaryPath) {
    $closedLoopSummary = Get-Content -LiteralPath $closedLoopSummaryPath -Raw -Encoding UTF8 | ConvertFrom-Json
}

$evidence = [pscustomobject]@{
    generatedAt = (Get-Date).ToUniversalTime().ToString('o')
    repository = (Get-RelativePath $repoRoot)
    recentCommits = $commits
    recentOpenSpecArchives = $archives
    ciJobs = @($workflowJobs)
    closedLoopSummary = $closedLoopSummary
    agentCapabilityMap = [ordered]@{
        specification = 'OpenSpec proposal/spec/tasks/archive 记录需求到规格的闭环'
        implementation = 'Git commits 按模块保留实现轨迹'
        verification = '质量门禁、OpenSpec、CTest 和 HTTP smoke 构成验证证据'
        delivery = 'GitHub Actions run 与推送历史构成交付证据'
        diagnosis = 'AI 诊断接口和工业闭环 demo 展示 agent 编排能力'
    }
}

$summaryJson = Join-Path $outputPath 'agent-workflow-evidence.json'
$evidence | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $summaryJson -Encoding UTF8

$markdownPath = Join-Path $outputPath 'agent-workflow-evidence.md'
$lines = New-Object System.Collections.Generic.List[string]
$lines.Add('# Agent 工作流证据包') | Out-Null
$lines.Add('') | Out-Null
$lines.Add("生成时间：$($evidence.generatedAt)") | Out-Null
$lines.Add('') | Out-Null
$lines.Add('## 能力映射') | Out-Null
foreach ($item in $evidence.agentCapabilityMap.GetEnumerator()) {
    $lines.Add("- $($item.Key)：$($item.Value)") | Out-Null
}
$lines.Add('') | Out-Null
$lines.Add('## 最近提交') | Out-Null
foreach ($commit in $commits) {
    $lines.Add("- `$($commit.sha)` $($commit.message) [$($commit.committedAt)]") | Out-Null
}
$lines.Add('') | Out-Null
$lines.Add('## 最近 OpenSpec 归档') | Out-Null
foreach ($archive in $archives) {
    $lines.Add("- $($archive.name)：proposal=$($archive.proposal)，tasks=$($archive.tasks)") | Out-Null
}
$lines.Add('') | Out-Null
$lines.Add('## CI 作业') | Out-Null
foreach ($job in $workflowJobs) {
    $lines.Add("- $job") | Out-Null
}
if ($closedLoopSummary -ne $null) {
    $lines.Add('') | Out-Null
    $lines.Add('## 工业闭环摘要') | Out-Null
    $closedLoopSummary.PSObject.Properties | ForEach-Object {
        $lines.Add("- $($_.Name)：$($_.Value)") | Out-Null
    }
}
$lines | Set-Content -LiteralPath $markdownPath -Encoding UTF8

Write-Host "已生成 Agent 证据包：$summaryJson"
Write-Host "已生成 Agent 证据说明：$markdownPath"