# Realtime Backup Module CMake Configuration

# 源文件
set(REALTIME_BACKUP_SOURCES
    RealtimeBackupMonitor.cpp
)

# 创建实时备份库
add_library(DataBackupRealtime ${REALTIME_BACKUP_SOURCES})

# 设置包含目录
target_include_directories(DataBackupRealtime
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/src
)

# 链接依赖库
target_link_libraries(DataBackupRealtime
    PUBLIC
        DataBackupCore
        DataBackupEncryption
)

# Windows特定设置
if(WIN32)
    target_compile_definitions(DataBackupRealtime PRIVATE _WIN32_WINNT=0x0601)
endif()

# 设置C++17标准
target_compile_features(DataBackupRealtime PUBLIC cxx_std_17)