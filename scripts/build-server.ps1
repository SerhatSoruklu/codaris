$ErrorActionPreference = "Stop"

$Root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$BuildDir = Join-Path $Root "build\native"

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
    "-DPostgreSQL_ROOT=$PostgreSqlRoot"

if ($LASTEXITCODE -ne 0) {
    throw "CMake configure failed with exit code $LASTEXITCODE"
}

cmake --build $BuildDir --config Debug
if ($LASTEXITCODE -ne 0) {
    throw "Native server build failed with exit code $LASTEXITCODE"
}

Write-Host "Server build complete: $BuildDir"
