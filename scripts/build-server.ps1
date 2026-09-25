param([string]$Mode = "")
$ErrorActionPreference = "Stop"

$Root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
if (-not $Mode) {
    $Mode = if ($env:CODARIS_ENV) { $env:CODARIS_ENV } elseif ($env:CODARIS_PRODUCTION -eq "1") { "production" } else { "development" }
}
if ($env:CODARIS_ENV_READY -ne "backend") {
    python (Join-Path $Root "scripts/project.py") $Mode build-server
    exit $LASTEXITCODE
}
$BuildDir = Join-Path $Root "build/native/$env:CODARIS_ENV"
$BuildType = if ($env:CODARIS_ENV -eq "production") { "Release" } else { "Debug" }

if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
    throw "CMake is not available on PATH. Install CMake, reopen PowerShell, and retry."
}

$PostgreSqlRoot = "C:\Program Files\PostgreSQL\18"
if ($env:PostgreSQL_ROOT) {
    $PostgreSqlRoot = $env:PostgreSQL_ROOT
}

cmake `
    -S $Root `
    -B $BuildDir `
    "-DPostgreSQL_ROOT=$PostgreSqlRoot" `
    "-DCMAKE_BUILD_TYPE=$BuildType"

if ($LASTEXITCODE -ne 0) {
    throw "CMake configure failed with exit code $LASTEXITCODE"
}

cmake --build $BuildDir --config $BuildType
if ($LASTEXITCODE -ne 0) {
    throw "Native server build failed with exit code $LASTEXITCODE"
}

Write-Host "Server build complete: $BuildDir"
