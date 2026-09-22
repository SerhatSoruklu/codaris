$ErrorActionPreference = "Stop"

$Root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$PostgreSqlBin = "C:\Program Files\PostgreSQL\18\bin"

if (Test-Path $PostgreSqlBin) {
    $env:Path = "$PostgreSqlBin;$env:Path"
}

$Candidates = @(
    (Join-Path $Root "build\native\Debug\codaris_server.exe"),
    (Join-Path $Root "build\native\codaris_server.exe")
)

$Executable = $null
foreach ($Candidate in $Candidates) {
    if (Test-Path $Candidate) {
        $Executable = $Candidate
        break
    }
}

if ($null -eq $Executable) {
    & (Join-Path $PSScriptRoot "build-server.ps1")

    foreach ($Candidate in $Candidates) {
        if (Test-Path $Candidate) {
            $Executable = $Candidate
            break
        }
    }
}

if ($null -eq $Executable) {
    throw "codaris_server.exe was not found after building."
}

if (-not $env:CODARIS_DATABASE_URL) {
    Write-Host "CODARIS_DATABASE_URL is not set. libpq will use PGHOST/PGPORT/PGDATABASE/PGUSER/PGPASSWORD if present."
}

& $Executable
exit $LASTEXITCODE
