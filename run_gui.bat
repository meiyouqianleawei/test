@echo off
echo ========================================
echo Data Backup Software GUI Launcher
echo ========================================
echo.

if not exist "build\src\ui\Release\DataBackupGUI.exe" (
    echo [ERROR] GUI executable not found!
    echo Please run build_gui.bat first.
    pause
    exit /b 1
)

if not exist "build\src\ui\Release\Qt6Core.dll" (
    echo [INFO] Deploying Qt dependencies...
    cd build\src\ui\Release
    D:\qt\6.11.1\msvc2022_64\bin\windeployqt.exe DataBackupGUI.exe
    cd ..\..\..\..
    echo [SUCCESS] Qt dependencies deployed.
    echo.
)

echo [INFO] Starting GUI...
start "" "build\src\ui\Release\DataBackupGUI.exe"

echo [SUCCESS] GUI started.
pause