# 加密策略模块库
add_library(DataBackupEncryption
    XOREncryption.cpp
)

target_include_directories(DataBackupEncryption PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
)