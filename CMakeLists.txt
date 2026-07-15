cmake_minimum_required(VERSION 3.15)
project(DataBackupSoftware VERSION 1.0.0 LANGUAGES CXX)

# C++17 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Zlib库配置（本地编译的静态库）
# 用户已下载zlib到：D:\软件开发实验\thirdparty\zlib\zlib-1.3.2
set(ZLIB_ROOT "${CMAKE_SOURCE_DIR}/thirdparty/zlib/zlib-1.3.2" CACHE PATH "Zlib root directory")
set(ZLIB_INCLUDE_DIR "${ZLIB_ROOT}" CACHE PATH "Zlib include directory")
set(ZLIB_LIBRARY "${ZLIB_ROOT}/build/Release/zs.lib" CACHE FILEPATH "Zlib static library")

# 验证zlib库是否存在
if(EXISTS "${ZLIB_LIBRARY}")
    message(STATUS "Zlib library found: ${ZLIB_LIBRARY}")
else()
    message(STATUS "Zlib library not found at: ${ZLIB_LIBRARY}")
    message(STATUS "Zlib compression will be disabled")
    unset(ZLIB_LIBRARY CACHE)
endif()

# 添加vcpkg工具链（如果使用vcpkg）
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE FILEPATH "vcpkg toolchain file")
endif()
set(CMAKE_CXX_EXTENSIONS OFF)

# 编译器特定配置
if(MSVC)
    # MSVC 编译器配置（推荐：完整支持 Windows API 和 C++17）
    add_compile_options(/W4 /WX- /utf-8)
    # 启用内存安全检测
    add_compile_options($<$<CONFIG:Debug>:/GS> $<$<CONFIG:Debug>:/sdl>)
    # 运行时库配置
    set_property(DIRECTORY PROPERTY CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # GCC 编译器配置
    # 注意：GCC 可能不完全支持 Windows-specific APIs (如 FILETIME, DWORD)
    message(WARNING "使用 GCC 编译器可能无法完全支持 Windows API。建议使用 MSVC (Visual Studio)。")
    add_compile_options(-Wall -Wextra -Wpedantic)
    # 启用 AddressSanitizer 用于内存泄漏检测（仅在 Debug 模式）
    add_compile_options($<$<CONFIG:Debug>:-fsanitize=address>)
    add_link_options($<$<CONFIG:Debug>:-fsanitize=address>)
endif()

# 输出编译器信息
message(STATUS "编译器: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "C++ 标准: ${CMAKE_CXX_STANDARD}")

# 查找第三方库（暂时注释，后续配置）
# find_package(ZLIB REQUIRED)
# find_package(LibLZMA REQUIRED)
# find_package(OpenSSL REQUIRED)

# Qt6 查找（可选，用于 GUI）
# 如果指定了 CMAKE_PREFIX_PATH 且路径指向 Qt，则尝试查找 Qt6
if(DEFINED CMAKE_PREFIX_PATH)
    # 尝试查找 Qt6（可选，即使找不到也不会报错）
    find_package(Qt6 COMPONENTS Core Gui Widgets QUIET)
    
    if(Qt6_FOUND)
        message(STATUS "Qt6 found at: ${Qt6_DIR}")
        message(STATUS "Qt6 version: ${Qt6_VERSION}")
        set(BUILD_GUI ON)
    else()
        message(STATUS "Qt6 not found (GUI will be disabled)")
        message(STATUS "To build GUI, specify Qt path: -DCMAKE_PREFIX_PATH=path/to/qt")
        set(BUILD_GUI OFF)
    endif()
else()
    message(STATUS "Qt6 not requested (GUI will be disabled)")
    message(STATUS "To build GUI, specify Qt path: -DCMAKE_PREFIX_PATH=path/to/qt")
    set(BUILD_GUI OFF)
endif()

# 添加子目录
add_subdirectory(src)
add_subdirectory(tests)

# 配置 Google Test (使用国内镜像源)
include(FetchContent)
FetchContent_Declare(
    googletest
    # 使用 Gitee 镜像源（国内访问更快）
    GIT_REPOSITORY https://gitee.com/mirrors/googletest.git
    GIT_TAG release-1.12.1
    # 备选方案：使用 GitHub（如果网络畅通）
    # GIT_REPOSITORY https://github.com/google/googletest.git
    # GIT_TAG release-1.12.1
)
# 对于 Windows：阻止 overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# 启用测试
enable_testing()

# 输出配置摘要
message(STATUS "")
message(STATUS "项目配置摘要:")
message(STATUS "  项目名称: ${PROJECT_NAME}")
message(STATUS "  项目版本: ${PROJECT_VERSION}")
message(STATUS "  构建类型: ${CMAKE_BUILD_TYPE}")
message(STATUS "  安装路径: ${CMAKE_INSTALL_PREFIX}")
if(BUILD_GUI)
    message(STATUS "  GUI: 启用")
else()
    message(STATUS "  GUI: 禁用")
endif()
message(STATUS "")