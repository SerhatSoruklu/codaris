param([string]$Mode = "")
$ErrorActionPreference = "Stop"

$Root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
if (-not $Mode) {
    $Mode = if ($env:CODARIS_ENV) { $env:CODARIS_ENV } elseif ($env:CODARIS_PRODUCTION -eq "1") { "production" } else { "development" }
}
if ($env:CODARIS_ENV_READY -ne "backend") {
    python (Join-Path $Root "scripts/project.py") $Mode migrate
    exit $LASTEXITCODE
}
$Migration = Join-Path $Root "db\migrations\001_initial.sql"
$PostgreSqlBin = "C:\Program Files\PostgreSQL\18\bin"

if (-not (Get-Command psql -ErrorAction SilentlyContinue) -and (Test-Path $PostgreSqlBin)) {
    $env:Path = "$PostgreSqlBin;$env:Path"
}

if (-not (Get-Command psql -ErrorAction SilentlyContinue)) {
    throw "psql was not found on PATH."
}

$HostName = if ($env:PGHOST) { $env:PGHOST } else { "localhost" }
$Port = if ($env:PGPORT) { $env:PGPORT } else { "5432" }
$Database = if ($env:PGDATABASE) { $env:PGDATABASE } else { "codaris" }
$User = if ($env:PGUSER) { $env:PGUSER } else { "codaris_app" }

$env:PGHOST = $HostName
$env:PGPORT = $Port
$env:PGDATABASE = $Database
$env:PGUSER = $User
python (Join-Path $Root "scripts/db-migrate.py")
if ($LASTEXITCODE -ne 0) { throw "Database migration failed" }
