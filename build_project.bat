@echo off
REM Build Project Script - DataBackupSoftware
REM Build using CMake and MSVC (Visual Studio)

echo ========================================
echo DataBackupSoftware Build Script
echo ========================================
echo.

echo [INFO] Using Visual Studio 2022 Generator (MSVC)
echo [INFO] This project requires MSVC for Windows API support
echo.

REM Create build directory
if not exist build (
    echo [INFO] Creating build directory...
    mkdir build
)

REM Enter build directory
cd build
echo [INFO] Current directory: %CD%
echo.

REM Configure CMake (Force Visual Studio Generator)
echo [INFO] Configuring CMake...
cmake .. -G "Visual Studio 17 2022" -A x64
if %errorlevel% neq 0 (
    echo [ERROR] CMake configuration failed!
    echo [ERROR] Possible reasons:
    echo   1. Visual Studio 2022 not installed
    echo   2. CMake version incompatible
    echo   3. Path encoding issues
    cd ..
    pause
    exit /b 1
)
echo.

REM Build project
echo [INFO] Building project...
cmake --build . --config Debug
if %errorlevel% neq 0 (
    echo [ERROR] Build failed!
    cd ..
    exit /b 1
)
echo.

echo ========================================
echo [SUCCESS] Build completed successfully!
echo ========================================
echo.
echo Next steps:
echo   1. Run tests: .\run_all_tests.bat
echo   2. Clean project: .\clean_project.bat
echo.

cd ..
pause