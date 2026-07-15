# 文件过滤器模块库
add_library(DataBackupFilter
    FilterChain.cpp
    PathFilter.cpp
    TypeFilter.cpp
    NameFilter.cpp
    TimeFilter.cpp
    SizeFilter.cpp
)

target_include_directories(DataBackupFilter PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(DataBackupFilter PUBLIC DataBackupCore)