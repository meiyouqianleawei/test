# 定时调度器模块库
add_library(DataBackupScheduler
    BackupScheduler.cpp
)

target_include_directories(DataBackupScheduler PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
)

# 链接其他模块
target_link_libraries(DataBackupScheduler PUBLIC
    DataBackupEngine
)