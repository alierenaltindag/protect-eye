@echo off
setlocal enabledelayedexpansion

echo ========================================================
echo        ProtectEye Windows Installer Build Script
echo ========================================================

REM 1. Check prerequisites
where cmake >nul 2>nul
if %errorlevel% neq 0 (
    echo [HATA] CMake bulunamadi! Lutfen PATH ortam degiskenine ekleyin.
    exit /b 1
)

where windeployqt >nul 2>nul
if %errorlevel% neq 0 (
    echo [UYARI] windeployqt bulunamadi! Qt bin dizinini PATH'e ekleyin.
)

set ROOT_DIR=%~dp0..\..
cd /d "%ROOT_DIR%"

echo.
echo [1/4] Proje Release modunda derleniyor...
cmake -B build_win -G "Ninja" -DCMAKE_BUILD_TYPE=Release
cmake --build build_win --config Release
if %errorlevel% neq 0 (
    echo [HATA] Derleme basarisiz oldu!
    exit /b 1
)

echo.
echo [2/4] Dagitim klasoru hazirlaniyor (build\dist)...
if exist "build\dist" rmdir /s /q "build\dist"
mkdir "build\dist"

copy "build_win\protecteye.exe" "build\dist\"
copy "LICENSE" "build\dist\"

echo.
echo [3/4] Qt6 bagimliliklari kopyalaniyor (windeployqt)...
windeployqt --release --no-translations --compiler-runtime "build\dist\protecteye.exe"

echo.
echo [4/4] Inno Setup ile Installer EXE paketi uretiliyor...
where iscc >nul 2>nul
if %errorlevel% equ 0 (
    iscc "packaging\windows\protecteye_setup.iss"
    echo.
    echo [BASARILI] Installer olusturuldu: build\windows_installer\ProtectEye_*_Setup.exe
) else (
    echo [BILGI] Inno Setup (iscc.exe) sistemde bulunamadi.
    echo "build\dist" dizini tasinabilir (portable) olarak kullanilabilir.
    echo Installer olusturmak icin Inno Setup 6 kurup 'packaging\windows\protecteye_setup.iss' dosyasini derleyin.
)

echo.
echo ========================================================
echo                  ISLEM TAMAMLANDI
echo ========================================================
