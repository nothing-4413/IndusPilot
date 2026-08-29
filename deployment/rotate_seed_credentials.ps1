param(
    [ValidateSet("admin", "operator", "maintainer")]
    [string]$Account = "admin",

    [Parameter(Mandatory = $true)]
    [SecureString]$NewPassword,

    [string]$EnvPath = (Join-Path $PSScriptRoot ".env"),
    [int]$Iterations = 120000
)

$ErrorActionPreference = "Stop"
if ($Iterations -lt 100000 -or $Iterations -gt 1000000) {
    throw "Iterations must be between 100000 and 1000000."
}

$newPasswordBstr = [Runtime.InteropServices.Marshal]::SecureStringToBSTR($NewPassword)
$plainPassword = [Runtime.InteropServices.Marshal]::PtrToStringBSTR($newPasswordBstr)
try {
    if ($plainPassword.Length -lt 12) {
        throw "The replacement password must contain at least 12 characters."
    }

    $salt = New-Object byte[] 16
    [Security.Cryptography.RandomNumberGenerator]::Fill($salt)
    $derive = [Security.Cryptography.Rfc2898DeriveBytes]::new(
        $plainPassword,
        $salt,
        $Iterations,
        [Security.Cryptography.HashAlgorithmName]::SHA256)
    try {
        $digest = $derive.GetBytes(32)
    } finally {
        $derive.Dispose()
    }

    $toHex = {
        param([byte[]]$Bytes)
        return -join ($Bytes | ForEach-Object { $_.ToString("x2") })
    }
    $hash = "pbkdf2_sha256`$$Iterations`$$( & $toHex $salt )`$$( & $toHex $digest )"
    $sql = "UPDATE users SET password_hash = '$hash', credential_version = credential_version + 1, requires_password_rotation = FALSE WHERE username = '$Account' AND enabled = TRUE AND requires_password_rotation = TRUE; SELECT ROW_COUNT();"

    if (-not (Get-Command docker -ErrorAction SilentlyContinue)) {
        throw "docker is required to run the rotation against the compose MySQL service."
    }
    if (-not (Test-Path -LiteralPath $EnvPath)) {
        throw "Compose environment file not found: $EnvPath"
    }
    $composeFile = Join-Path $PSScriptRoot "docker-compose.yml"
    $sql | docker compose --env-file $EnvPath -f $composeFile exec -T mysql sh -c 'MYSQL_PWD="$MYSQL_ROOT_PASSWORD" mysql --protocol=TCP -h127.0.0.1 -uroot "$MYSQL_DATABASE"'
    if ($LASTEXITCODE -ne 0) {
        throw "MySQL rejected the credential rotation."
    }
    Write-Host "Seed credential rotation completed for account '$Account'."
} finally {
    $plainPassword = $null
    [Runtime.InteropServices.Marshal]::ZeroFreeBSTR($newPasswordBstr)
}
