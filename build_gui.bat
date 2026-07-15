@echo off
chcp 65001 >nul
echo ========================================
echo 构建 Qt GUI 应用程序
echo ========================================
echo.

REM 检查构建目录是否存在
if not exist build (
    echo [INFO] 创建构建目录...
    mkdir build
)

REM 进入构建目录
cd build

echo [1/2] 运行 CMake 配置...
cmake .. -G "Visual Studio 17 2022" -A x64 ^
    -DCMAKE_PREFIX_PATH="D:/Qt/6.11.1/msvc2022_64" ^
    -DBUILD_SHARED_LIBS=OFF

if errorlevel 1 (
    echo.
    echo [ERROR] CMake 配置失败！
    echo 请检查：
    echo 1. Qt6 是否正确安装（路径：C:/Qt/6.6.0/msvc2019_64）
    echo 2. CMAKE_PREFIX_PATH 是否正确设置
    echo 3. Visual Studio 2022 是否正确安装
    pause
    exit /b 1
)

echo.
echo [2/2] 编译项目（Release 配置）...
cmake --build . --config Release --target DataBackupGUI

if errorlevel 1 (
    echo.
    echo [ERROR] 编译失败！
    pause
    exit /b 1
)

echo.
echo ========================================
echo [SUCCESS] GUI 构建完成！
echo ========================================
echo.
echo 可执行文件位置：
echo   build\src\ui\Release\DataBackupGUI.exe
echo.
echo 运行方式：
echo   1. 直接运行：build\src\ui\Release\DataBackupGUI.exe
echo   2. 或使用：run_gui.bat
echo.
pause