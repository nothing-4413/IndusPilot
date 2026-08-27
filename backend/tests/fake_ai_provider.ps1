param(
    [Parameter(Mandatory = $true)]
    [int]$Port,

    [Parameter(Mandatory = $true)]
    [string]$RequestLog,

    [Parameter(Mandatory = $true)]
    [string]$ReadyFile,

    [ValidateSet("success", "failure", "non-json", "timeout", "retry-success", "oversized")]
    [string]$Mode = "success",

    [int]$DelayMs = 500
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$listener = New-Object System.Net.Sockets.TcpListener -ArgumentList ([System.Net.IPAddress]::Loopback), $Port

function Write-RequestRecord {
    param([string]$RequestText)

    $separator = $RequestText.IndexOf("`r`n`r`n", [System.StringComparison]::Ordinal)
    $headerText = if ($separator -ge 0) { $RequestText.Substring(0, $separator) } else { $RequestText }
    $lines = $headerText -split "`r`n"
    $requestLine = $lines[0] -split " "
    $headers = [ordered]@{}
    for ($index = 1; $index -lt $lines.Count; $index++) {
        $headerSeparator = $lines[$index].IndexOf(':')
        if ($headerSeparator -gt 0) {
            $headerName = $lines[$index].Substring(0, $headerSeparator)
            $headers[$headerName] = $lines[$index].Substring($headerSeparator + 1).Trim()
        }
    }
    $record = [ordered]@{
        method = $requestLine[0]
        path = ([Uri]("http://127.0.0.1" + $requestLine[1])).AbsolutePath
        headers = $headers
        body = $RequestText.Substring($separator + 4)
    }
    Add-Content -LiteralPath $RequestLog -Value ($record | ConvertTo-Json -Compress -Depth 8) -Encoding UTF8
}

try {
    $listener.Start()
    [System.IO.File]::WriteAllText($ReadyFile, "ready", [System.Text.Encoding]::UTF8)
    $requestCount = 0
    while ($true) {
        $client = $listener.AcceptTcpClient()
        $stream = $null
        try {
            $stream = $client.GetStream()
            $stream.ReadTimeout = 10000
            $buffer = New-Object byte[] 4096
            $requestBytes = New-Object System.IO.MemoryStream
            $headerEnd = -1
            $contentLength = 0
            while ($headerEnd -lt 0) {
                $read = $stream.Read($buffer, 0, $buffer.Length)
                if ($read -le 0) { break }
                $requestBytes.Write($buffer, 0, $read)
                $headerCandidate = [System.Text.Encoding]::ASCII.GetString($requestBytes.ToArray())
                $headerEnd = $headerCandidate.IndexOf("`r`n`r`n", [System.StringComparison]::Ordinal)
                if ($headerEnd -ge 0) {
                    $headerLines = $headerCandidate.Substring(0, $headerEnd) -split "`r`n"
                    foreach ($headerLine in $headerLines) {
                        if ($headerLine -match "^Content-Length:\s*(\d+)$") {
                            $contentLength = [int]$Matches[1]
                        }
                    }
                }
            }
            $bodyStart = $headerEnd + 4
            while ($headerEnd -ge 0 -and $requestBytes.Length - $bodyStart -lt $contentLength) {
                $read = $stream.Read($buffer, 0, $buffer.Length)
                if ($read -le 0) { break }
                $requestBytes.Write($buffer, 0, $read)
            }
            $requestText = [System.Text.Encoding]::UTF8.GetString($requestBytes.ToArray())
            $requestCount++
            Write-RequestRecord $requestText
            switch ($Mode) {
                "success" {
                    $statusLine = "200 OK"
                    $contentType = "application/json"
                    $responseBody = '{"content":"fake provider response"}'
                }
                "failure" {
                    $statusLine = "503 Service Unavailable"
                    $contentType = "application/json"
                    $responseBody = '{"error":"fake provider unavailable"}'
                }
                "retry-success" {
                    if ($requestCount -eq 1) {
                        $statusLine = "503 Service Unavailable"
                        $contentType = "application/json"
                        $responseBody = '{"error":"fake provider transient failure"}'
                    } else {
                        $statusLine = "200 OK"
                        $contentType = "application/json"
                        $responseBody = '{"content":"retry provider response"}'
                    }
                }
                "non-json" {
                    $statusLine = "200 OK"
                    $contentType = "text/plain"
                    $responseBody = "fake provider non-json response"
                }
                "timeout" {
                    Start-Sleep -Milliseconds $DelayMs
                    $statusLine = "200 OK"
                    $contentType = "application/json"
                    $responseBody = '{"content":"late fake provider response"}'
                }
                "oversized" {
                    $statusLine = "200 OK"
                    $contentType = "application/json"
                    $responseBody = '{"content":"' + ("x" * 2048) + '"}'
                }
            }
            $responseBytes = [System.Text.Encoding]::UTF8.GetBytes($responseBody)
            $responseHeaders = "HTTP/1.1 $statusLine`r`nContent-Type: $contentType`r`nContent-Length: $($responseBytes.Length)`r`nConnection: close`r`n`r`n"
            $headerBytes = [System.Text.Encoding]::ASCII.GetBytes($responseHeaders)
            $stream.Write($headerBytes, 0, $headerBytes.Length)
            $stream.Write($responseBytes, 0, $responseBytes.Length)
        } catch {
            # The backend may close a timed-out request before the fake provider responds.
        } finally {
            if ($null -ne $stream) {
                $stream.Dispose()
            }
            $client.Close()
        }
    }
} finally {
    $listener.Stop()
}
