$ErrorActionPreference = "Stop"

$Root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$BuildDir = Join-Path $Root "build\client"

if (-not (Get-Command emcc -ErrorAction SilentlyContinue)) {
    $Emsdk = $env:EMSDK
    if ([string]::IsNullOrWhiteSpace($Emsdk)) {
        $Emsdk = "C:\emsdk"
    }

    $EnvScript = Join-Path $Emsdk "emsdk_env.ps1"
    if (-not (Test-Path $EnvScript)) {
        throw "emcc is not on PATH and emsdk_env.ps1 was not found at $EnvScript"
    }

    . $EnvScript
}

New-Item -ItemType Directory -Force $BuildDir | Out-Null


& emcc `
    (Join-Path $Root "src\client\main.c") `
    -std=c17 `
    -O2 `
    -Wall -Wextra -Wpedantic `
    -sDYNAMIC_EXECUTION=0 -sSTACK_OVERFLOW_CHECK=2 -sNO_EXIT_RUNTIME=1 `
    -sEXPORTED_RUNTIME_METHODS=ccall `
    -sASSERTIONS=0 `
    -sENVIRONMENT=web `
    -o (Join-Path $BuildDir "codaris.js")

if ($LASTEXITCODE -ne 0) {
    throw "Client build failed with exit code $LASTEXITCODE"
}

Copy-Item (Join-Path $Root "web\styles.css") $BuildDir -Force
Copy-Item (Join-Path $Root "web\host.js") $BuildDir -Force
New-Item -ItemType Directory -Force (Join-Path $BuildDir "assets") | Out-Null
Copy-Item (Join-Path $Root "web\assets\*.svg") (Join-Path $BuildDir "assets") -Force


& python (Join-Path $Root "scripts\build-pages.py")
if ($LASTEXITCODE -ne 0) { throw "Static page build failed" }

Write-Host "Client build complete: $BuildDir"
