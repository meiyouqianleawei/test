@echo off
chcp 65001 >nul
echo ========================================
echo Running All Unit Tests
echo ========================================
echo.

echo [1/10] Running Framework Tests...
.\build\tests\Debug\test_framework_basic.exe
if errorlevel 1 (
    echo [ERROR] Framework tests failed!
    pause
    exit /b 1
)
echo.

echo [2/10] Running FileScanner Tests...
.\build\tests\Debug\test_filescanner.exe
if errorlevel 1 (
    echo [ERROR] FileScanner tests failed!
    pause
    exit /b 1
)
echo.

echo [3/10] Running MetadataManager Tests...
.\build\tests\Debug\test_metadatamanager.exe
if errorlevel 1 (
    echo [ERROR] MetadataManager tests failed!
    pause
    exit /b 1
)
echo.

echo [4/10] Running Packager Tests...
.\build\tests\Debug\test_packager.exe
if errorlevel 1 (
    echo [ERROR] Packager tests failed!
    pause
    exit /b 1
)
echo.

echo [5/10] Running Filter Tests...
.\build\tests\Debug\test_filters.exe
if errorlevel 1 (
    echo [ERROR] Filter tests failed!
    pause
    exit /b 1
)
echo.

echo [6/10] Running Huffman Compression Tests...
.\build\tests\Debug\test_huffman_compression.exe
if errorlevel 1 (
    echo [ERROR] Huffman compression tests failed!
    pause
    exit /b 1
)
echo.

echo [7/10] Running XOR Encryption Tests...
.\build\tests\Debug\test_xor_encryption.exe
if errorlevel 1 (
    echo [ERROR] XOR encryption tests failed!
    pause
    exit /b 1
)
echo.

echo [8/10] Running Backup Engine Tests...
.\build\tests\Debug\test_backup_engine.exe
if errorlevel 1 (
    echo [ERROR] Backup engine tests failed!
    pause
    exit /b 1
)
echo.

echo [9/10] Running Backup Scheduler Tests...
.\build\tests\Debug\test_backup_scheduler.exe
if errorlevel 1 (
    echo [ERROR] Backup scheduler tests failed!
    pause
    exit /b 1
)
echo.

echo [10/11] Running Zlib Compression Tests...
if exist .\build\tests\Debug\test_zlib_compression.exe (
    .\build\tests\Debug\test_zlib_compression.exe
    if errorlevel 1 (
        echo [ERROR] Zlib compression tests failed!
        pause
        exit /b 1
    )
) else (
    echo [INFO] Zlib compression tests skipped (zlib not configured)
)
echo.

echo [11/11] Running Realtime Backup Tests...
.\build\tests\Debug\test_realtime_backup.exe
if errorlevel 1 (
    echo [ERROR] Realtime backup tests failed!
    pause
    exit /b 1
)
echo.

echo ========================================
echo [SUCCESS] All available tests passed!
echo ========================================
echo.
echo Test Summary:
echo   - Framework Tests: 8 tests
echo   - FileScanner Tests: 9 tests
echo   - MetadataManager Tests: 12 tests
echo   - Packager Tests: 14 tests
echo   - Filter Tests: 37 tests
echo   - Huffman Compression Tests: 13 tests
echo   - XOR Encryption Tests: 12 tests
echo   - Backup Engine Tests: 13 tests
echo   - Backup Scheduler Tests: 17 tests
echo   - Zlib Compression Tests: 10 tests (if available)
echo   - Realtime Backup Tests: 17 tests
echo.
pause