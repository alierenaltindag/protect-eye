# ProtectEye Windows Packaging Automation Script
param (
    [string]$QtDir = "",
    [string]$InnoSetupDir = "C:\Program Files (x86)\Inno Setup 6"
)

$ErrorActionPreference = "Stop"
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Definition
$RootDir = Resolve-Path "$ScriptDir\..\.."

Write-Host "==> ProtectEye Windows Build & Packaging Automation" -ForegroundColor Cyan

# Check CMake
if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
    Write-Error "CMake is not installed or not in PATH."
}

# Optional QtDir
if ($QtDir -and (Test-Path $QtDir)) {
    $env:PATH = "$QtDir\bin;$env:PATH"
}

cd $RootDir

# Step 1: Configure & Build
Write-Host "==> Step 1: Building ProtectEye..." -ForegroundColor Yellow
cmake -B build_win -G "Ninja" -DCMAKE_BUILD_TYPE=Release
cmake --build build_win --config Release

# Step 2: Assemble distribution directory
Write-Host "==> Step 2: Assembling distribution files..." -ForegroundColor Yellow
$DistDir = "$RootDir\build\dist"
if (Test-Path $DistDir) { Remove-Item -Recurse -Force $DistDir }
New-Item -ItemType Directory -Path $DistDir | Out-Null

Copy-Item "$RootDir\build_win\protecteye.exe" "$DistDir\"
Copy-Item "$RootDir\LICENSE" "$DistDir\"

# Step 3: Run windeployqt
Write-Host "==> Step 3: Running windeployqt for Qt6 dependencies..." -ForegroundColor Yellow
if (Get-Command windeployqt -ErrorAction SilentlyContinue) {
    & windeployqt --release --no-translations --compiler-runtime "$DistDir\protecteye.exe"
    $qtBin = Split-Path (Get-Command windeployqt).Source
    $qtRoot = Split-Path $qtBin
    $tlsSource = Join-Path $qtRoot "plugins\tls"
    if (Test-Path $tlsSource) {
        $tlsTarget = Join-Path $DistDir "tls"
        if (-not (Test-Path $tlsTarget)) { New-Item -ItemType Directory -Path $tlsTarget | Out-Null }
        Copy-Item -Path "$tlsSource\*.dll" -Destination $tlsTarget -Force
    }
    Get-ChildItem -Path $qtBin -Filter "libcrypto*.dll" -ErrorAction SilentlyContinue | Copy-Item -Destination $DistDir -Force
    Get-ChildItem -Path $qtBin -Filter "libssl*.dll" -ErrorAction SilentlyContinue | Copy-Item -Destination $DistDir -Force
} else {
    Write-Warning "windeployqt not found in PATH. Please ensure Qt DLLs are bundled."
}

# Step 4: Run Inno Setup
Write-Host "==> Step 4: Generating Inno Setup Installer..." -ForegroundColor Yellow
$Iscc = Get-Command iscc -ErrorAction SilentlyContinue
if (-not $Iscc -and (Test-Path "$InnoSetupDir\ISCC.exe")) {
    $Iscc = "$InnoSetupDir\ISCC.exe"
}

if ($Iscc) {
    & $Iscc "$ScriptDir\protecteye_setup.iss"
    Write-Host "==> SUCCESS: Installer created in build\windows_installer\ProtectEye_*_Setup.exe" -ForegroundColor Green
} else {
    Write-Warning "Inno Setup (iscc) not found. Portable files are ready in build\dist."
}
