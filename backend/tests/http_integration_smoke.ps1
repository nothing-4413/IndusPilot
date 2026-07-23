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
    $health = $null
    for ($attempt = 0; $attempt -lt 20; $attempt++) {
        try {
            $health = Invoke-RestMethod -Uri "$BaseUrl/health" -Method Get -Headers @{ "X-Trace-Id" = "trace-it-health" } -TimeoutSec 2
            break
        } catch {
            if ($proc.HasExited) {
                throw
            }
            Start-Sleep -Milliseconds 500
        }
    }
    Assert-True ($null -ne $health) "Backend did not become ready."

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
    Invoke-ExpectStatus -Uri "$BaseUrl/api/v1/monitoring/states" -Method Post -Status 401 -Body '{"assetId":"asset-it-001","state":"online"}'

    $emptyMonitoring = Invoke-RestMethod -Uri "$BaseUrl/api/v1/monitoring/states" -Method Get -Headers $operatorHeaders -TimeoutSec 10
    Assert-True $emptyMonitoring.success "Monitoring state list failed."
    Assert-True ($null -ne $emptyMonitoring.data.summary.states.online) "Monitoring state summary was not returned."

    Invoke-ExpectStatus -Uri "$BaseUrl/api/v1/monitoring/states" -Method Post -Status 400 -Headers $operatorHeaders -Body '{"assetId":"asset-it-001"}'
    Invoke-ExpectStatus -Uri "$BaseUrl/api/v1/monitoring/states" -Method Post -Status 400 -Headers $operatorHeaders -Body '{"assetId":"asset-it-001","state":"invalid"}'
    Invoke-ExpectStatus -Uri "$BaseUrl/api/v1/monitoring/states" -Method Post -Status 400 -Headers $operatorHeaders -Body '{"assetId":"asset-it-001","state":"online","severity":"invalid"}'

    $assets = Invoke-RestMethod -Uri "$BaseUrl/api/v1/assets" -Method Get -Headers $operatorHeaders -TimeoutSec 10
    Assert-True $assets.success "Protected asset read failed."
    Invoke-ExpectStatus -Uri "$BaseUrl/api/v1/assets" -Method Post -Status 403 -Headers $operatorHeaders -Body '{"id":"asset-it-denied","name":"denied"}'

    $adminLogin = Invoke-RestMethod -Uri "$BaseUrl/api/v1/auth/login" -Method Post -ContentType "application/json" -Body '{"username":"admin","password":"admin123"}' -TimeoutSec 10
    Assert-True $adminLogin.success "Admin login failed."
    $adminToken = $adminLogin.data.token
    $adminHeaders = @{ Authorization = "Bearer $adminToken"; "X-Trace-Id" = "trace-it-admin" }

    Invoke-ExpectStatus -Uri "$BaseUrl/api/v1/audit/events" -Method Get -Status 403 -Headers $operatorHeaders
    $loginAudit = Invoke-RestMethod -Uri "$BaseUrl/api/v1/audit/events" -Method Get -Headers $adminHeaders -TimeoutSec 10
    Assert-True $loginAudit.success "Operation audit query failed."
    $adminLoginAudit = @($loginAudit.data) | Where-Object { $_.actor -eq "admin" -and $_.action -eq "auth.login" }
    Assert-True (@($adminLoginAudit).Count -ge 1) "Admin login audit event was not recorded."

    Invoke-ExpectStatus -Uri "$BaseUrl/api/v1/assets" -Method Post -Status 400 -Headers $adminHeaders -Body '{"name":"missing id"}'
    Invoke-ExpectStatus -Uri "$BaseUrl/api/v1/assets/not-exist" -Method Get -Status 404 -Headers $adminHeaders

    Invoke-ExpectStatus -Uri "$BaseUrl/api/v1/assets?status=unknown" -Method Get -Status 400 -Headers $adminHeaders
    Invoke-ExpectStatus -Uri "$BaseUrl/api/v1/assets" -Method Post -Status 400 -Headers $adminHeaders -Body '{"id":"asset-it-bad","name":"bad","status":"unknown"}'

    $assetBody = '{"id":"asset-it-001","name":"integration asset","type":"pump","factory":"factory-it","workshop":"workshop-it","productionLine":"line-it","status":"active"}'
    $created = Invoke-RestMethod -Uri "$BaseUrl/api/v1/assets" -Method Post -Headers $adminHeaders -ContentType "application/json" -Body $assetBody -TimeoutSec 10
    Assert-True $created.success "Admin asset creation failed."
    Assert-True ($created.data.status -eq "active") "Created asset status was not returned."

    $filtered = Invoke-RestMethod -Uri "$BaseUrl/api/v1/assets?factory=factory-it&workshop=workshop-it&productionLine=line-it&status=active" -Method Get -Headers $adminHeaders -TimeoutSec 10
    $matched = @($filtered.data) | Where-Object { $_.id -eq "asset-it-001" }
    Assert-True (@($matched).Count -eq 1) "Asset hierarchy filter failed."

    Invoke-ExpectStatus -Uri "$BaseUrl/api/v1/assets/asset-it-001/status" -Method Patch -Status 403 -Headers $operatorHeaders -Body '{"status":"maintenance"}'
    Invoke-ExpectStatus -Uri "$BaseUrl/api/v1/assets/asset-it-001/status" -Method Patch -Status 400 -Headers $adminHeaders -Body '{"status":"unknown"}'
    Invoke-ExpectStatus -Uri "$BaseUrl/api/v1/assets/not-exist/status" -Method Patch -Status 404 -Headers $adminHeaders -Body '{"status":"maintenance"}'

    $updated = Invoke-RestMethod -Uri "$BaseUrl/api/v1/assets/asset-it-001/status" -Method Patch -Headers $adminHeaders -ContentType "application/json" -Body '{"status":"maintenance"}' -TimeoutSec 10
    Assert-True $updated.success "Asset lifecycle update failed."
    Assert-True ($updated.data.status -eq "maintenance") "Asset lifecycle status was not updated."

    $maintenanceAssets = Invoke-RestMethod -Uri "$BaseUrl/api/v1/assets?status=maintenance" -Method Get -Headers $adminHeaders -TimeoutSec 10
    $maintenanceMatch = @($maintenanceAssets.data) | Where-Object { $_.id -eq "asset-it-001" }
    Assert-True (@($maintenanceMatch).Count -eq 1) "Asset status filter failed."
    $runtimeBody = '{"assetId":"asset-it-001","state":"critical","metricSummary":"temperature high","severity":"critical"}'
    $runtime = Invoke-RestMethod -Uri "$BaseUrl/api/v1/monitoring/states" -Method Post -Headers $operatorHeaders -ContentType "application/json" -Body $runtimeBody -TimeoutSec 10
    Assert-True $runtime.success "Runtime state update failed."
    Assert-True ($runtime.data.assetId -eq "asset-it-001") "Runtime state asset id was not returned."
    Assert-True ($runtime.data.state -eq "critical") "Runtime state value was not returned."
    Assert-True ($runtime.data.severity -eq "critical") "Runtime severity was not returned."
    Assert-True (-not [string]::IsNullOrWhiteSpace($runtime.data.updatedAt)) "Runtime updatedAt was not generated."

    $runtimeDetail = Invoke-RestMethod -Uri "$BaseUrl/api/v1/monitoring/states/asset-it-001" -Method Get -Headers $operatorHeaders -TimeoutSec 10
    Assert-True $runtimeDetail.success "Runtime state detail failed."
    Assert-True ($runtimeDetail.data.metricSummary -eq "temperature high") "Runtime state detail did not match."

    $monitoringList = Invoke-RestMethod -Uri "$BaseUrl/api/v1/monitoring/states" -Method Get -Headers $operatorHeaders -TimeoutSec 10
    Assert-True ($monitoringList.data.summary.states.critical -ge 1) "Runtime state summary did not include critical state."
    Assert-True ($monitoringList.data.summary.severity.critical -ge 1) "Runtime severity summary did not include critical severity."

    Invoke-ExpectStatus -Uri "$BaseUrl/api/v1/monitoring/states/not-exist" -Method Get -Status 404 -Headers $operatorHeaders

    $maintainerLogin = Invoke-RestMethod -Uri "$BaseUrl/api/v1/auth/login" -Method Post -ContentType "application/json" -Body '{"username":"maintainer","password":"maintainer123"}' -TimeoutSec 10
    Assert-True $maintainerLogin.success "Maintainer login failed."
    $maintainerHeaders = @{ Authorization = "Bearer $($maintainerLogin.data.token)"; "X-Trace-Id" = "trace-it-maintainer" }
    Invoke-ExpectStatus -Uri "$BaseUrl/api/v1/monitoring/states" -Method Post -Status 403 -Headers $maintainerHeaders -Body '{"assetId":"asset-it-001","state":"online"}'
    Invoke-ExpectStatus -Uri "$BaseUrl/api/v1/alerts" -Method Post -Status 403 -Headers $maintainerHeaders -Body '{"id":"alert-denied","assetId":"asset-it-001","severity":"warning","title":"denied"}'
    Invoke-ExpectStatus -Uri "$BaseUrl/api/v1/alerts" -Method Post -Status 400 -Headers $operatorHeaders -Body '{"id":"alert-bad","assetId":"asset-it-001","severity":"bad","title":"bad"}'
    Invoke-ExpectStatus -Uri "$BaseUrl/api/v1/alerts" -Method Post -Status 400 -Headers $operatorHeaders -Body '{"id":"alert-bad-state","assetId":"asset-it-001","severity":"warning","state":"bad","title":"bad"}'
    Invoke-ExpectStatus -Uri "$BaseUrl/api/v1/alert-rules" -Method Post -Status 403 -Headers $maintainerHeaders -Body '{"id":"rule-denied","name":"denied","minSeverity":"warning","channel":"console","target":"lead"}'
    Invoke-ExpectStatus -Uri "$BaseUrl/api/v1/alert-rules" -Method Post -Status 400 -Headers $operatorHeaders -Body '{"id":"rule-bad","name":"bad","minSeverity":"bad","channel":"console","target":"lead"}'

    $alertRuleBody = '{"id":"rule-it-critical","name":"critical alert fanout","assetId":"asset-it-001","minSeverity":"warning","channel":"console","target":"shift-lead","enabled":true}'
    $alertRule = Invoke-RestMethod -Uri "$BaseUrl/api/v1/alert-rules" -Method Post -Headers $operatorHeaders -ContentType "application/json" -Body $alertRuleBody -TimeoutSec 10
    Assert-True $alertRule.success "Alert rule creation failed."
    Assert-True ($alertRule.data.id -eq "rule-it-critical") "Alert rule id did not match."
    Assert-True ($alertRule.data.enabled -eq $true) "Alert rule enabled state did not match."

    $alertRules = Invoke-RestMethod -Uri "$BaseUrl/api/v1/alert-rules" -Method Get -Headers $operatorHeaders -TimeoutSec 10
    $alertRuleMatch = @($alertRules.data) | Where-Object { $_.id -eq "rule-it-critical" }
    Assert-True (@($alertRuleMatch).Count -eq 1) "Alert rule query failed."
    $alertBody = '{"id":"alert-it-001","assetId":"asset-it-001","severity":"critical","title":"temperature critical"}'
    $alert = Invoke-RestMethod -Uri "$BaseUrl/api/v1/alerts" -Method Post -Headers $operatorHeaders -ContentType "application/json" -Body $alertBody -TimeoutSec 10
    Assert-True $alert.success "Alert creation failed."
    Assert-True ($alert.data.state -eq "open") "Created alert was not open."

    $alertDetail = Invoke-RestMethod -Uri "$BaseUrl/api/v1/alerts/alert-it-001" -Method Get -Headers $operatorHeaders -TimeoutSec 10
    Assert-True $alertDetail.success "Alert detail failed."
    Assert-True ($alertDetail.data.severity -eq "critical") "Alert severity did not match."

    $criticalAlerts = Invoke-RestMethod -Uri "$BaseUrl/api/v1/alerts?assetId=asset-it-001&severity=critical&state=open" -Method Get -Headers $operatorHeaders -TimeoutSec 10
    $criticalMatch = @($criticalAlerts.data) | Where-Object { $_.id -eq "alert-it-001" }
    Assert-True (@($criticalMatch).Count -eq 1) "Alert filtering failed."

    $notifications = Invoke-RestMethod -Uri "$BaseUrl/api/v1/alert-notifications" -Method Get -Headers $operatorHeaders -TimeoutSec 10
    $notificationMatch = @($notifications.data) | Where-Object { $_.alertId -eq "alert-it-001" -and $_.ruleId -eq "rule-it-critical" }
    Assert-True (@($notificationMatch).Count -eq 1) "Alert notification fanout failed."
    Assert-True ($notificationMatch[0].channel -eq "console") "Alert notification channel did not match."
    Assert-True ($notificationMatch[0].target -eq "shift-lead") "Alert notification target did not match."
    Assert-True ($notificationMatch[0].status -eq "queued") "Alert notification status did not match."
    Assert-True ($notificationMatch[0].attemptCount -eq 0) "Alert notification attempt count should start at zero."

    $dispatch = Invoke-RestMethod -Uri "$BaseUrl/api/v1/alert-notifications/dispatch" -Method Post -Headers $operatorHeaders -TimeoutSec 10
    Assert-True $dispatch.success "Alert notification dispatch failed."
    Assert-True ($dispatch.data.sent -ge 1) "Alert notification dispatch did not send queued notification."

    $sentNotifications = Invoke-RestMethod -Uri "$BaseUrl/api/v1/alert-notifications" -Method Get -Headers $operatorHeaders -TimeoutSec 10
    $sentNotificationMatch = @($sentNotifications.data) | Where-Object { $_.alertId -eq "alert-it-001" -and $_.ruleId -eq "rule-it-critical" }
    Assert-True (@($sentNotificationMatch).Count -eq 1) "Sent alert notification query failed."
    Assert-True ($sentNotificationMatch[0].status -eq "sent") "Alert notification was not marked sent."
    Assert-True ($sentNotificationMatch[0].attemptCount -eq 1) "Alert notification attempt count did not increment."
    Assert-True (-not [string]::IsNullOrWhiteSpace($sentNotificationMatch[0].deliveredAt)) "Alert notification deliveredAt was not set."
    Invoke-ExpectStatus -Uri "$BaseUrl/api/v1/alert-notifications/not-exist/retry" -Method Post -Status 404 -Headers $operatorHeaders
    $dispatchAudit = Invoke-RestMethod -Uri "$BaseUrl/api/v1/audit/events" -Method Get -Headers $adminHeaders -TimeoutSec 10
    $dispatchAuditMatch = @($dispatchAudit.data) | Where-Object { $_.actor -eq "operator" -and $_.action -eq "alert-notification.dispatch" }
    Assert-True (@($dispatchAuditMatch).Count -ge 1) "Alert notification dispatch audit event was not recorded."
    Invoke-ExpectStatus -Uri "$BaseUrl/api/v1/alerts/alert-it-001/assign" -Method Post -Status 400 -Headers $operatorHeaders -Body '{}'
    Invoke-ExpectStatus -Uri "$BaseUrl/api/v1/alerts/not-exist/acknowledge" -Method Post -Status 404 -Headers $operatorHeaders

    $acknowledged = Invoke-RestMethod -Uri "$BaseUrl/api/v1/alerts/alert-it-001/acknowledge" -Method Post -Headers $operatorHeaders -TimeoutSec 10
    Assert-True ($acknowledged.data.state -eq "acknowledged") "Alert acknowledgement failed."
    Assert-True ($acknowledged.data.acknowledgedBy -eq "operator") "Alert acknowledgement user did not match."

    $assigned = Invoke-RestMethod -Uri "$BaseUrl/api/v1/alerts/alert-it-001/assign" -Method Post -Headers $operatorHeaders -ContentType "application/json" -Body '{"assignee":"maintainer"}' -TimeoutSec 10
    Assert-True ($assigned.data.state -eq "assigned") "Alert assignment failed."
    Assert-True ($assigned.data.assignedTo -eq "maintainer") "Alert assignee did not match."

    $resolved = Invoke-RestMethod -Uri "$BaseUrl/api/v1/alerts/alert-it-001/resolve" -Method Post -Headers $operatorHeaders -TimeoutSec 10
    Assert-True ($resolved.data.state -eq "resolved") "Alert resolve failed."

    $closed = Invoke-RestMethod -Uri "$BaseUrl/api/v1/alerts/alert-it-001/close" -Method Post -Headers $operatorHeaders -TimeoutSec 10
    Assert-True ($closed.data.state -eq "closed") "Alert close failed."
    Invoke-ExpectStatus -Uri "$BaseUrl/api/v1/work-orders" -Method Post -Status 403 -Headers $operatorHeaders -Body '{"id":"wo-denied","assetId":"asset-it-001","summary":"denied"}'
    Invoke-ExpectStatus -Uri "$BaseUrl/api/v1/work-orders" -Method Post -Status 400 -Headers $maintainerHeaders -Body '{"id":"wo-bad","summary":"missing asset"}'
    Invoke-ExpectStatus -Uri "$BaseUrl/api/v1/work-orders" -Method Post -Status 400 -Headers $maintainerHeaders -Body '{"id":"wo-bad-state","assetId":"asset-it-001","summary":"bad","state":"invalid"}'

    $workOrderBody = '{"id":"wo-it-001","assetId":"asset-it-001","alertId":"alert-it-001","summary":"inspect pump"}'
    $workOrder = Invoke-RestMethod -Uri "$BaseUrl/api/v1/work-orders" -Method Post -Headers $maintainerHeaders -ContentType "application/json" -Body $workOrderBody -TimeoutSec 10
    Assert-True $workOrder.success "Work order creation failed."
    Assert-True ($workOrder.data.state -eq "created") "Created work order was not created state."

    $workOrderDetail = Invoke-RestMethod -Uri "$BaseUrl/api/v1/work-orders/wo-it-001" -Method Get -Headers $maintainerHeaders -TimeoutSec 10
    Assert-True $workOrderDetail.success "Work order detail failed."
    Assert-True ($workOrderDetail.data.alertId -eq "alert-it-001") "Work order alert id did not match."

    $updatedWorkOrder = Invoke-RestMethod -Uri "$BaseUrl/api/v1/work-orders/wo-it-001" -Method Patch -Headers $maintainerHeaders -ContentType "application/json" -Body '{"summary":"updated bearing inspection","assignee":"maintainer","result":"pending vibration check"}' -TimeoutSec 10
    Assert-True $updatedWorkOrder.success "Work order update failed."
    Assert-True ($updatedWorkOrder.data.summary -eq "updated bearing inspection") "Work order summary was not updated."
    Assert-True ($updatedWorkOrder.data.assignee -eq "maintainer") "Work order assignee was not updated."
    Assert-True ($updatedWorkOrder.data.result -eq "pending vibration check") "Work order result was not updated."
    Invoke-ExpectStatus -Uri "$BaseUrl/api/v1/work-orders/wo-it-001" -Method Patch -Status 400 -Headers $maintainerHeaders -Body '{}'

    $attachmentBody = '{"id":"work-order-attachment-it-001","fileName":"bearing-photo.jpg","uri":"file:///evidence/bearing-photo.jpg","contentType":"image/jpeg","sizeBytes":2048}'
    $attachment = Invoke-RestMethod -Uri "$BaseUrl/api/v1/work-orders/wo-it-001/attachments" -Method Post -Headers $maintainerHeaders -ContentType "application/json" -Body $attachmentBody -TimeoutSec 10
    Assert-True $attachment.success "Work order attachment registration failed."
    Assert-True ($attachment.data.fileName -eq "bearing-photo.jpg") "Work order attachment file name did not match."
    Assert-True ($attachment.data.uploadedBy -eq "maintainer") "Work order attachment uploader did not default to session user."
    $attachments = Invoke-RestMethod -Uri "$BaseUrl/api/v1/work-orders/wo-it-001/attachments" -Method Get -Headers $operatorHeaders -TimeoutSec 10
    $attachmentMatch = @($attachments.data) | Where-Object { $_.id -eq "work-order-attachment-it-001" }
    Assert-True (@($attachmentMatch).Count -eq 1) "Work order attachment query failed."
    Invoke-ExpectStatus -Uri "$BaseUrl/api/v1/work-orders/wo-it-001/attachments" -Method Post -Status 400 -Headers $maintainerHeaders -Body '{"id":"missing-uri","fileName":"missing.txt"}'
    $filteredOrders = Invoke-RestMethod -Uri "$BaseUrl/api/v1/work-orders?assetId=asset-it-001&alertId=alert-it-001&state=assigned" -Method Get -Headers $maintainerHeaders -TimeoutSec 10
    $filteredOrderMatch = @($filteredOrders.data) | Where-Object { $_.id -eq "wo-it-001" }
    Assert-True (@($filteredOrderMatch).Count -eq 1) "Work order filtering failed."

    Invoke-ExpectStatus -Uri "$BaseUrl/api/v1/work-orders/not-exist" -Method Get -Status 404 -Headers $maintainerHeaders
    Invoke-ExpectStatus -Uri "$BaseUrl/api/v1/work-orders/wo-it-001/assign" -Method Post -Status 400 -Headers $maintainerHeaders -Body '{}'
    Invoke-ExpectStatus -Uri "$BaseUrl/api/v1/work-orders/not-exist/start" -Method Post -Status 404 -Headers $maintainerHeaders

    $assignedOrder = Invoke-RestMethod -Uri "$BaseUrl/api/v1/work-orders/wo-it-001/assign" -Method Post -Headers $maintainerHeaders -ContentType "application/json" -Body '{"assignee":"maintainer"}' -TimeoutSec 10
    Assert-True ($assignedOrder.data.state -eq "assigned") "Work order assignment failed."

    $processingOrder = Invoke-RestMethod -Uri "$BaseUrl/api/v1/work-orders/wo-it-001/start" -Method Post -Headers $maintainerHeaders -TimeoutSec 10
    Assert-True ($processingOrder.data.state -eq "processing") "Work order start failed."

    Invoke-ExpectStatus -Uri "$BaseUrl/api/v1/work-orders/wo-it-001/complete" -Method Post -Status 400 -Headers $maintainerHeaders -Body '{}'
    $completedOrder = Invoke-RestMethod -Uri "$BaseUrl/api/v1/work-orders/wo-it-001/complete" -Method Post -Headers $maintainerHeaders -ContentType "application/json" -Body '{"result":"bearing replaced"}' -TimeoutSec 10
    Assert-True ($completedOrder.data.state -eq "completed") "Work order completion failed."
    Assert-True ($completedOrder.data.result -eq "bearing replaced") "Work order result did not match."

    $closedOrder = Invoke-RestMethod -Uri "$BaseUrl/api/v1/work-orders/wo-it-001/close" -Method Post -Headers $maintainerHeaders -TimeoutSec 10
    Assert-True ($closedOrder.data.state -eq "closed") "Work order close failed."

    $history = Invoke-RestMethod -Uri "$BaseUrl/api/v1/assets/asset-it-001/maintenance-history" -Method Get -Headers $maintainerHeaders -TimeoutSec 10
    $historyMatch = @($history.data) | Where-Object { $_.id -eq "wo-it-001" }
    Assert-True (@($historyMatch).Count -eq 1) "Maintenance history failed."

    $fromAlert = Invoke-RestMethod -Uri "$BaseUrl/api/v1/work-orders/from-alert" -Method Post -Headers $maintainerHeaders -ContentType "application/json" -Body '{"alertId":"alert-it-001","summary":"follow up from alert"}' -TimeoutSec 10
    Assert-True $fromAlert.success "Work order from alert failed."
    Assert-True ($fromAlert.data.alertId -eq "alert-it-001") "Work order from alert did not link alert."
    Invoke-ExpectStatus -Uri "$BaseUrl/api/v1/work-orders/from-alert" -Method Post -Status 404 -Headers $maintainerHeaders -Body '{"alertId":"not-exist","summary":"missing alert"}'
    Invoke-ExpectStatus -Uri "$BaseUrl/api/v1/ai/troubleshoot" -Method Post -Status 401 -Body '{"relatedType":"alert","relatedId":"alert-it-001","prompt":"explain"}'
    Invoke-ExpectStatus -Uri "$BaseUrl/api/v1/ai/troubleshoot" -Method Post -Status 400 -Headers $operatorHeaders -Body '{"relatedType":"alert"}'
    Invoke-ExpectStatus -Uri "$BaseUrl/api/v1/ai/diagnose" -Method Post -Status 401 -Body '{"relatedType":"alert","relatedId":"alert-it-001","prompt":"diagnose"}'
    Invoke-ExpectStatus -Uri "$BaseUrl/api/v1/ai/diagnose" -Method Post -Status 400 -Headers $operatorHeaders -Body '{"relatedType":"alert"}'

    $aiStatus = Invoke-RestMethod -Uri "$BaseUrl/api/v1/ai/status" -Method Get -Headers $operatorHeaders -TimeoutSec 10
    Assert-True $aiStatus.success "AI status failed."
    Assert-True (-not [string]::IsNullOrWhiteSpace($aiStatus.data.provider)) "AI provider was not returned."

    $aiBody = '{"relatedType":"alert","relatedId":"alert-it-001","prompt":"explain critical temperature","contextItems":["asset-it-001","temperature high"]}'
    $aiSuggestion = Invoke-RestMethod -Uri "$BaseUrl/api/v1/ai/troubleshoot" -Method Post -Headers $operatorHeaders -ContentType "application/json" -Body $aiBody -TimeoutSec 10
    Assert-True $aiSuggestion.success "AI troubleshooting failed."
    Assert-True (-not $aiSuggestion.data.available) "AI troubleshooting should report unavailable in local mode."
    Assert-True (-not [string]::IsNullOrWhiteSpace($aiSuggestion.data.content)) "AI troubleshooting content was empty."

    $diagnosisBody = '{"relatedType":"alert","relatedId":"alert-it-001","prompt":"diagnose critical temperature","context":{"assetId":"asset-it-001","alertTitle":"temperature critical","runtimeState":"critical","severity":"critical","metricSummary":"temperature high","workOrderHistory":"bearing replaced","operatorDescription":"smell detected","contextItems":["asset-it-001","temperature high"]}}'
    $diagnosis = Invoke-RestMethod -Uri "$BaseUrl/api/v1/ai/diagnose" -Method Post -Headers $operatorHeaders -ContentType "application/json" -Body $diagnosisBody -TimeoutSec 10
    Assert-True $diagnosis.success "AI diagnosis failed."
    Assert-True ($diagnosis.data.riskLevel -eq "critical") "AI diagnosis risk level did not match."
    Assert-True $diagnosis.data.requiresHumanReview "AI diagnosis should require human review."
    Assert-True (@($diagnosis.data.possibleCauses).Count -ge 1) "AI diagnosis possible causes were empty."
    Assert-True (@($diagnosis.data.recommendedActions).Count -ge 1) "AI diagnosis actions were empty."

    $logSummary = Invoke-RestMethod -Uri "$BaseUrl/api/v1/ai/summarize-logs" -Method Post -Headers $operatorHeaders -ContentType "application/json" -Body '{"relatedType":"work-order","relatedId":"wo-it-001","prompt":"summarize maintenance log"}' -TimeoutSec 10
    Assert-True $logSummary.success "AI log summary failed."
    Assert-True (-not $logSummary.data.available) "AI log summary should report unavailable in local mode."

    $aiInteractions = Invoke-RestMethod -Uri "$BaseUrl/api/v1/ai/interactions?relatedType=alert&relatedId=alert-it-001" -Method Get -Headers $operatorHeaders -TimeoutSec 10
    $aiMatch = @($aiInteractions.data) | Where-Object { $_.relatedId -eq "alert-it-001" }
    Assert-True (@($aiMatch).Count -ge 2) "AI interaction audit query failed."

    $aiPage = Invoke-RestMethod -Uri "$BaseUrl/api/v1/ai/interactions?relatedType=alert&relatedId=alert-it-001&limit=1&offset=0" -Method Get -Headers $operatorHeaders -TimeoutSec 10
    Assert-True ($aiPage.success) "AI interaction audit page query failed."
    Assert-True (@($aiPage.data.items).Count -eq 1) "AI interaction audit page size did not match."
    Assert-True ($aiPage.data.total -ge @($aiMatch).Count) "AI interaction audit page total was not returned."
    Assert-True ($aiPage.data.limit -eq 1) "AI interaction audit page limit did not match."
    Assert-True ($aiPage.data.offset -eq 0) "AI interaction audit page offset did not match."
    Invoke-ExpectStatus -Uri "$BaseUrl/api/v1/ai/interactions?limit=0&offset=0" -Method Get -Status 400 -Headers $operatorHeaders
    Invoke-ExpectStatus -Uri "$BaseUrl/api/v1/ai/interactions?limit=1&offset=-1" -Method Get -Status 400 -Headers $operatorHeaders
} finally {
    if ($proc -and -not $proc.HasExited) {
        Stop-Process -Id $proc.Id -Force
        Wait-Process -Id $proc.Id -Timeout 10 -ErrorAction SilentlyContinue
    }
    $env:INDUSPILOT_SERVER_PORT = $oldPort
    $env:INDUSPILOT_REDIS_SESSION_STORE = $oldSessionStore
    Remove-Item -LiteralPath $stdout, $stderr -Force -ErrorAction SilentlyContinue
}