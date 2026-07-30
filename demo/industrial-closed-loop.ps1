param(
    [string]$BaseUrl = "http://127.0.0.1:8080",
    [string]$OutputDir = "demo-output",
    [string]$AdminUser = "admin",
    [string]$AdminPassword = "admin123",
    [string]$OperatorUser = "operator",
    [string]$OperatorPassword = "operator123",
    [string]$MaintainerUser = "maintainer",
    [string]$MaintainerPassword = "maintainer123"
)

$ErrorActionPreference = "Stop"

function Write-Step {
    param([string]$Message)
    Write-Host "[IndusPilot Demo] $Message"
}

function Invoke-JsonApi {
    param(
        [string]$Method,
        [string]$Path,
        [hashtable]$Headers = @{},
        [object]$Body = $null
    )

    $uri = "$BaseUrl$Path"
    if ($null -eq $Body) {
        return Invoke-RestMethod -Uri $uri -Method $Method -Headers $Headers -TimeoutSec 15
    }

    $jsonBody = $Body
    if ($Body -isnot [string]) {
        $jsonBody = $Body | ConvertTo-Json -Depth 8 -Compress
    }
    return Invoke-RestMethod -Uri $uri -Method $Method -Headers $Headers -ContentType "application/json" -Body $jsonBody -TimeoutSec 15
}

function Assert-Success {
    param(
        [object]$Response,
        [string]$Message
    )
    if (-not $Response.success) {
        throw $Message
    }
}

function Login-User {
    param(
        [string]$Username,
        [string]$Password,
        [string]$TraceId
    )

    $response = Invoke-JsonApi -Method Post -Path "/api/v1/auth/login" -Headers @{ "X-Trace-Id" = $TraceId } -Body @{ username = $Username; password = $Password }
    Assert-Success $response "用户 $Username 登录失败。"
    return @{ Authorization = "Bearer $($response.data.token)"; "X-Trace-Id" = $TraceId }
}

New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null
$runId = Get-Date -Format "yyyyMMddHHmmss"
$assetId = "asset-demo-$runId"
$alertId = "alert-demo-$runId"
$ruleId = "rule-demo-$runId"
$workOrderId = "wo-demo-$runId"
$attachmentId = "wo-attachment-demo-$runId"

Write-Step "检查后端健康状态"
$health = Invoke-JsonApi -Method Get -Path "/health" -Headers @{ "X-Trace-Id" = "trace-demo-health-$runId" }
$health | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $OutputDir "01-health.json") -Encoding UTF8

Write-Step "登录管理员、操作员、维修员"
$adminHeaders = Login-User -Username $AdminUser -Password $AdminPassword -TraceId "trace-demo-admin-$runId"
$operatorHeaders = Login-User -Username $OperatorUser -Password $OperatorPassword -TraceId "trace-demo-operator-$runId"
$maintainerHeaders = Login-User -Username $MaintainerUser -Password $MaintainerPassword -TraceId "trace-demo-maintainer-$runId"

Write-Step "创建工业资产 $assetId"
$asset = Invoke-JsonApi -Method Post -Path "/api/v1/assets" -Headers $adminHeaders -Body @{
    id = $assetId
    name = "Demo Pump $runId"
    type = "pump"
    factory = "factory-demo"
    workshop = "workshop-a"
    productionLine = "line-01"
    status = "active"
}
Assert-Success $asset "资产创建失败。"
$asset | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $OutputDir "02-asset.json") -Encoding UTF8

Write-Step "写入运行状态：温度和振动异常"
$runtime = Invoke-JsonApi -Method Post -Path "/api/v1/monitoring/states" -Headers $operatorHeaders -Body @{
    assetId = $assetId
    state = "critical"
    metricSummary = "temperature=96C; vibration=8.7mm/s; bearing noise detected"
    severity = "critical"
}
Assert-Success $runtime "运行状态写入失败。"
$runtime | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $OutputDir "03-runtime-state.json") -Encoding UTF8

Write-Step "创建告警规则并触发关键告警 $alertId"
$rule = Invoke-JsonApi -Method Post -Path "/api/v1/alert-rules" -Headers $operatorHeaders -Body @{
    id = $ruleId
    name = "Demo critical fanout"
    assetId = $assetId
    minSeverity = "warning"
    channel = "console"
    target = "shift-lead"
    enabled = $true
}
Assert-Success $rule "告警规则创建失败。"
$alert = Invoke-JsonApi -Method Post -Path "/api/v1/alerts" -Headers $operatorHeaders -Body @{
    id = $alertId
    assetId = $assetId
    severity = "critical"
    title = "Demo pump temperature critical"
}
Assert-Success $alert "告警创建失败。"
$alert | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $OutputDir "04-alert.json") -Encoding UTF8

Write-Step "派发告警通知"
$dispatch = Invoke-JsonApi -Method Post -Path "/api/v1/alert-notifications/dispatch" -Headers $operatorHeaders
Assert-Success $dispatch "告警通知派发失败。"
$dispatch | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $OutputDir "05-notification-dispatch.json") -Encoding UTF8

Write-Step "调用 AI 诊断，生成现场处置建议"
$diagnosis = Invoke-JsonApi -Method Post -Path "/api/v1/ai/diagnose" -Headers $operatorHeaders -Body @{
    relatedType = "alert"
    relatedId = $alertId
    prompt = "diagnose demo pump critical temperature and vibration"
    context = @{
        assetId = $assetId
        alertTitle = "Demo pump temperature critical"
        runtimeState = "critical"
        severity = "critical"
        metricSummary = "temperature=96C; vibration=8.7mm/s; bearing noise detected"
        workOrderHistory = "last bearing replacement 90 days ago"
        operatorDescription = "现场闻到异味，泵体温度明显升高"
        contextItems = @($assetId, $alertId, "temperature critical", "vibration high")
    }
}
Assert-Success $diagnosis "AI 诊断失败。"
$diagnosis | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $OutputDir "06-ai-diagnosis.json") -Encoding UTF8

Write-Step "告警确认、分派并生成维护工单 $workOrderId"
$acknowledged = Invoke-JsonApi -Method Post -Path "/api/v1/alerts/$alertId/acknowledge" -Headers $operatorHeaders
Assert-Success $acknowledged "告警确认失败。"
$assignedAlert = Invoke-JsonApi -Method Post -Path "/api/v1/alerts/$alertId/assign" -Headers $operatorHeaders -Body @{ assignee = $MaintainerUser }
Assert-Success $assignedAlert "告警分派失败。"
$workOrder = Invoke-JsonApi -Method Post -Path "/api/v1/work-orders" -Headers $maintainerHeaders -Body @{
    id = $workOrderId
    assetId = $assetId
    alertId = $alertId
    summary = "Inspect demo pump bearing and cooling loop"
}
Assert-Success $workOrder "工单创建失败。"
$workOrder | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $OutputDir "07-work-order-created.json") -Encoding UTF8

Write-Step "登记现场附件并流转工单"
$attachment = Invoke-JsonApi -Method Post -Path "/api/v1/work-orders/$workOrderId/attachments" -Headers $maintainerHeaders -Body @{
    id = $attachmentId
    fileName = "demo-bearing-photo.jpg"
    uri = "file:///demo/evidence/demo-bearing-photo.jpg"
    contentType = "image/jpeg"
    sizeBytes = 4096
}
Assert-Success $attachment "附件登记失败。"
$assignedOrder = Invoke-JsonApi -Method Post -Path "/api/v1/work-orders/$workOrderId/assign" -Headers $maintainerHeaders -Body @{ assignee = $MaintainerUser }
Assert-Success $assignedOrder "工单分派失败。"
$processingOrder = Invoke-JsonApi -Method Post -Path "/api/v1/work-orders/$workOrderId/start" -Headers $maintainerHeaders
Assert-Success $processingOrder "工单开工失败。"
$completedOrder = Invoke-JsonApi -Method Post -Path "/api/v1/work-orders/$workOrderId/complete" -Headers $maintainerHeaders -Body @{ result = "清理冷却风道，更换轴承润滑脂，复核温度恢复正常" }
Assert-Success $completedOrder "工单完成失败。"
$closedOrder = Invoke-JsonApi -Method Post -Path "/api/v1/work-orders/$workOrderId/close" -Headers $maintainerHeaders
Assert-Success $closedOrder "工单关闭失败。"
$closedOrder | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $OutputDir "08-work-order-closed.json") -Encoding UTF8

Write-Step "关闭告警并导出审计 CSV"
$resolved = Invoke-JsonApi -Method Post -Path "/api/v1/alerts/$alertId/resolve" -Headers $operatorHeaders
Assert-Success $resolved "告警解决失败。"
$closedAlert = Invoke-JsonApi -Method Post -Path "/api/v1/alerts/$alertId/close" -Headers $operatorHeaders
Assert-Success $closedAlert "告警关闭失败。"
$closedAlert | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $OutputDir "09-alert-closed.json") -Encoding UTF8

$auditCsv = Invoke-RestMethod -Uri "$BaseUrl/api/v1/audit/events/export?resourceId=$alertId" -Method Get -Headers $adminHeaders -TimeoutSec 15
$auditPath = Join-Path $OutputDir "10-audit-export.csv"
$auditCsv | Set-Content -LiteralPath $auditPath -Encoding UTF8

$summary = [ordered]@{
    runId = $runId
    baseUrl = $BaseUrl
    assetId = $assetId
    alertId = $alertId
    ruleId = $ruleId
    workOrderId = $workOrderId
    riskLevel = $diagnosis.data.riskLevel
    requiresHumanReview = $diagnosis.data.requiresHumanReview
    auditExport = $auditPath
}
$summary | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $OutputDir "00-summary.json") -Encoding UTF8
Write-Step "演示完成：输出目录 $OutputDir"
$summary