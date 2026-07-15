# 备份引擎模块库
add_library(DataBackupEngine
    BackupEngine.cpp
    RestoreEngine.cpp
)

target_include_directories(DataBackupEngine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
)

# 链接其他模块
target_link_libraries(DataBackupEngine PUBLIC
    DataBackupCore
    DataBackupPack
    DataBackupFilter
    DataBackupCompression
    DataBackupEncryption
)