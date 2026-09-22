$checks = @(
    @{ Name = "Git"; Command = "git"; Args = @("--version") },
    @{ Name = "VS Code"; Command = "code"; Args = @("--version") },
    @{ Name = "CMake"; Command = "cmake"; Args = @("--version") },
    @{ Name = "MSVC compiler"; Command = "cl"; Args = @() },
    @{ Name = "Emscripten"; Command = "emcc"; Args = @("--version") },
    @{ Name = "PostgreSQL client"; Command = "psql"; Args = @("--version") }
)

foreach ($check in $checks) {
    $cmd = Get-Command $check.Command -ErrorAction SilentlyContinue
    if ($cmd) {
        Write-Host "[OK] $($check.Name): $($cmd.Source)"
    }
    else {
        Write-Host "[MISSING/NOT ON PATH] $($check.Name)"
    }
}

$pgRoot = "C:\Program Files\PostgreSQL\18"
if (Test-Path $pgRoot) {
    Write-Host "[OK] PostgreSQL 18 install directory: $pgRoot"
}
else {
    Write-Host "[CHECK] PostgreSQL 18 not found at the default CODARIS path: $pgRoot"
}

$emsdk = "C:\emsdk"
if (Test-Path $emsdk) {
    Write-Host "[OK] Emscripten SDK directory: $emsdk"
}
else {
    Write-Host "[CHECK] Emscripten SDK not found at the default CODARIS path: $emsdk"
}
