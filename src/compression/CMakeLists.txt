# 压缩策略模块库 - 支持多种压缩算法

# Huffman压缩（总是编译，无外部依赖）
set(COMPRESSION_SOURCES
    HuffmanCompression.cpp
)

# Zlib压缩（条件编译，需要zlib库）
if(DEFINED ZLIB_LIBRARY)
    list(APPEND COMPRESSION_SOURCES ZlibCompression.cpp)
    message(STATUS "Zlib compression enabled: ${ZLIB_LIBRARY}")
else()
    message(STATUS "Zlib compression disabled (ZLIB_LIBRARY not defined)")
    message(STATUS "Only Huffman compression will be available")
endif()

# 创建压缩库
add_library(DataBackupCompression ${COMPRESSION_SOURCES})

target_include_directories(DataBackupCompression PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
)

# 条件链接Zlib库（如果可用）
if(DEFINED ZLIB_LIBRARY)
    target_include_directories(DataBackupCompression PUBLIC ${ZLIB_INCLUDE_DIR})
    target_link_libraries(DataBackupCompression PUBLIC ${ZLIB_LIBRARY})
endif()