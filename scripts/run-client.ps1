$ErrorActionPreference = "Stop"

$Root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$Index = Join-Path $Root "build\client\index.html"

& (Join-Path $PSScriptRoot "build-client.ps1")

if (-not (Get-Command emrun -ErrorAction SilentlyContinue)) {
    $Emsdk = $env:EMSDK
    if ([string]::IsNullOrWhiteSpace($Emsdk)) {
        $Emsdk = "C:\emsdk"
    }

    $EnvScript = Join-Path $Emsdk "emsdk_env.ps1"
    if (-not (Test-Path $EnvScript)) {
        throw "emrun is not on PATH and emsdk_env.ps1 was not found at $EnvScript"
    }

    . $EnvScript
}

& emrun --browser chrome $Index
