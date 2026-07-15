@echo off
REM Clean Project Script - DataBackupSoftware
REM Clean all build files and temporary files

echo ========================================
echo DataBackupSoftware Clean Script
echo ========================================
echo.

REM Clean build directory
if exist build (
    echo [INFO] Removing build directory...
    rmdir /s /q build
    echo [SUCCESS] Build directory removed.
) else (
    echo [INFO] Build directory not found. Already clean.
)
echo.

REM Clean CMake cache files
echo [INFO] Cleaning CMake cache files...
if exist CMakeCache.txt del CMakeCache.txt
if exist CMakeFiles rmdir /s /q CMakeFiles
if exist cmake_install.cmake del cmake_install.cmake
if exist Makefile del Makefile
echo [SUCCESS] CMake cache files cleaned.
echo.

REM Clean temporary files
echo [INFO] Cleaning temporary files...
del /s /q *.log 2>nul
del /s /q *.tmp 2>nul
del /s /q *.temp 2>nul
echo [SUCCESS] Temporary files cleaned.
echo.

echo ========================================
echo [SUCCESS] Project cleaned successfully!
echo ========================================
echo.
echo To rebuild, run: .\build_project.bat
echo.

pause