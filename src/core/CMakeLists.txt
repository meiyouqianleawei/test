# 核心模块库
add_library(DataBackupCore
    FileInfo.cpp
    FileScanner.cpp  # 文件扫描器（硬链接/软链接处理）
    MetadataManager.cpp  # 元数据管理（时间戳、属性）
    # 其他源文件将在后续添加
    # BackupEngine.cpp
    # RestoreEngine.cpp
)

target_include_directories(DataBackupCore PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
)

# 链接 Windows API 库（仅 Windows 平台）
if(WIN32)
    target_link_libraries(DataBackupCore PRIVATE
        # Windows API 可能需要的库
        # kernel32
    )
endif()