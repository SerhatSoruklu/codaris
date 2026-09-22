$ErrorActionPreference = "Stop"

$Root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
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

& psql `
    -h $HostName `
    -p $Port `
    -U $User `
    -d $Database `
    -v ON_ERROR_STOP=1 `
    -f $Migration

if ($LASTEXITCODE -ne 0) {
    throw "Database migration failed with exit code $LASTEXITCODE"
}
