# Qt GUI 项目 CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

# 项目名称
project(DataBackupGUI VERSION 1.0.0 LANGUAGES CXX)

# C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 自动处理 MOC、UIC、RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# 查找 Qt6
find_package(Qt6 REQUIRED COMPONENTS Widgets Core Gui)

# 源文件
set(GUI_SOURCES
    main.cpp
    MainWindow.cpp
    MainWindow.h
    BackupTab.cpp
    BackupTab.h
    RestoreTab.cpp
    RestoreTab.h
    SchedulerTab.cpp
    SchedulerTab.h
    RealtimeTab.cpp
    RealtimeTab.h
    BackupWorker.cpp
    BackupWorker.h
    RestoreWorker.cpp
    RestoreWorker.h
    SchedulerWorker.cpp
    SchedulerWorker.h
    RealtimeWorker.cpp
    RealtimeWorker.h
    FilterDialog.cpp
    FilterDialog.h
)

# 创建可执行文件
add_executable(DataBackupGUI ${GUI_SOURCES})

# 链接 Qt 库
target_link_libraries(DataBackupGUI
    PRIVATE
    Qt6::Widgets
    Qt6::Core
    Qt6::Gui
)

# 链接项目库
target_link_libraries(DataBackupGUI
    PRIVATE
    DataBackupCore
    DataBackupEngine
    DataBackupScheduler
    DataBackupRealtime
    DataBackupCompression
    DataBackupEncryption
    DataBackupFilter
    DataBackupPack
)

# 包含目录
target_include_directories(DataBackupGUI
    PRIVATE
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src/scheduler
    ${CMAKE_SOURCE_DIR}/src/realtime
    ${CMAKE_SOURCE_DIR}/src/compression
    ${CMAKE_SOURCE_DIR}/src/encryption
    ${CMAKE_SOURCE_DIR}/src/filter
    ${CMAKE_SOURCE_DIR}/src/pack
)

# Windows 特定设置
if(WIN32)
    # 设置为 Windows GUI 应用（不显示控制台窗口）
    set_target_properties(DataBackupGUI PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

# 安装规则
install(TARGETS DataBackupGUI
    RUNTIME DESTINATION bin
)