param([string]$Mode = "")
$ErrorActionPreference = "Stop"
$Root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
if (-not $Mode) {
    $Mode = if ($env:CODARIS_ENV) { $env:CODARIS_ENV } elseif ($env:CODARIS_PRODUCTION -eq "1") { "production" } else { "development" }
}
if ($env:CODARIS_ENV_READY -ne "frontend") {
    python (Join-Path $Root "scripts/project.py") $Mode run-client
    exit $LASTEXITCODE
}
& (Join-Path $PSScriptRoot "build-client.ps1")
if ($LASTEXITCODE -ne 0) { throw "Client build failed" }
python (Join-Path $Root "scripts/serve-dev.py") --port $env:CODARIS_FRONTEND_PORT --api-port $env:CODARIS_API_PORT
