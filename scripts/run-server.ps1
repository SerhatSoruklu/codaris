param([string]$Mode = "")
$ErrorActionPreference = "Stop"

$Root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
if (-not $Mode) {
    $Mode = if ($env:CODARIS_ENV) { $env:CODARIS_ENV } elseif ($env:CODARIS_PRODUCTION -eq "1") { "production" } else { "development" }
}
if ($env:CODARIS_ENV_READY -ne "backend") {
    python (Join-Path $Root "scripts/project.py") $Mode run-server
    exit $LASTEXITCODE
}
$PostgreSqlBin = "C:\Program Files\PostgreSQL\18\bin"

if (Test-Path $PostgreSqlBin) {
    $env:Path = "$PostgreSqlBin;$env:Path"
}

& (Join-Path $PSScriptRoot "build-server.ps1")
if ($LASTEXITCODE -ne 0) { throw "Backend build failed" }
$BuildType = if ($env:CODARIS_ENV -eq "production") { "Release" } else { "Debug" }
$Base = Join-Path $Root "build/native/$env:CODARIS_ENV"
$Candidates = @((Join-Path $Base "$BuildType/codaris_server.exe"), (Join-Path $Base "codaris_server.exe"))
$Executable = $Candidates | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $Executable) { throw "Selected backend binary was not found" }
& $Executable
exit $LASTEXITCODE
