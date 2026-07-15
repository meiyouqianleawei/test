# 文件打包模块库
add_library(DataBackupPack
    Packager.cpp  # 文件打包器（头部+索引+数据区）
)

target_include_directories(DataBackupPack PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
)

# Packager 依赖:加密、压缩、Core(MetadataManager)
target_link_libraries(DataBackupPack PUBLIC
    DataBackupEncryption
    DataBackupCompression
    DataBackupCore
)