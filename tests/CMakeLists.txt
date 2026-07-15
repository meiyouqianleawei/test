# Google Test 配置

# 链接已实现的模块库
set(TEST_LIBRARIES
    DataBackupCore
    DataBackupPack
    DataBackupFilter
    DataBackupCompression
    DataBackupEncryption
    DataBackupEngine
    DataBackupScheduler
    DataBackupRealtime
    gtest_main
)

# 基础测试 - 测试框架验证
add_executable(test_framework_basic test_framework_basic.cpp)
target_link_libraries(test_framework_basic ${TEST_LIBRARIES})

# FileScanner 测试 - 文件扫描器（重点模块）
add_executable(test_filescanner test_filescanner.cpp)
target_link_libraries(test_filescanner ${TEST_LIBRARIES})

# MetadataManager 测试 - 元数据管理器
add_executable(test_metadatamanager test_metadatamanager.cpp)
target_link_libraries(test_metadatamanager ${TEST_LIBRARIES})

# Packager 测试 - 文件打包器
add_executable(test_packager test_packager.cpp)
target_link_libraries(test_packager ${TEST_LIBRARIES})

# Filter 测试 - 文件过滤器链
add_executable(test_filters test_filters.cpp)
target_link_libraries(test_filters ${TEST_LIBRARIES})

# Integration 测试 - 集成测试
add_executable(test_integration test_integration.cpp)
target_link_libraries(test_integration ${TEST_LIBRARIES})

# Real Directory 测试 - 真实目录测试工具
add_executable(test_real_directory test_real_directory.cpp)
target_link_libraries(test_real_directory ${TEST_LIBRARIES})

# Huffman Compression 测试 - Huffman压缩模块
add_executable(test_huffman_compression test_huffman_compression.cpp)
target_link_libraries(test_huffman_compression ${TEST_LIBRARIES})

# Zlib Compression 测试 - Zlib压缩模块（条件编译）
if(DEFINED ZLIB_LIBRARY)
    add_executable(test_zlib_compression test_zlib_compression.cpp)
    target_link_libraries(test_zlib_compression ${TEST_LIBRARIES})
    set(ZLIB_TEST_AVAILABLE ON)
    message(STATUS "Zlib compression test enabled")
else()
    message(STATUS "Zlib compression test disabled (ZLIB_LIBRARY not defined)")
    set(ZLIB_TEST_AVAILABLE OFF)
endif()

# XOR Encryption 测试 - XOR加密模块
add_executable(test_xor_encryption test_xor_encryption.cpp)
target_link_libraries(test_xor_encryption ${TEST_LIBRARIES})

# Backup Engine 测试 - 备份引擎
add_executable(test_backup_engine test_backup_engine.cpp)
target_link_libraries(test_backup_engine ${TEST_LIBRARIES})

# Backup Scheduler 测试 - 定时调度器
add_executable(test_backup_scheduler test_backup_scheduler.cpp)
target_link_libraries(test_backup_scheduler ${TEST_LIBRARIES})

# Realtime Backup 测试 - 实时备份监控
add_executable(test_realtime_backup test_realtime_backup.cpp)
target_link_libraries(test_realtime_backup ${TEST_LIBRARIES})


# 添加测试（兼容 Visual Studio）
include(GoogleTest)

# 方式 1：使用 gtest_discover_tests
gtest_discover_tests(test_framework_basic DISCOVERY_MODE PRE_TEST)
gtest_discover_tests(test_filescanner DISCOVERY_MODE PRE_TEST)
gtest_discover_tests(test_metadatamanager DISCOVERY_MODE PRE_TEST)
gtest_discover_tests(test_packager DISCOVERY_MODE PRE_TEST)
gtest_discover_tests(test_filters DISCOVERY_MODE PRE_TEST)
gtest_discover_tests(test_integration DISCOVERY_MODE PRE_TEST)
gtest_discover_tests(test_huffman_compression DISCOVERY_MODE PRE_TEST)
gtest_discover_tests(test_xor_encryption DISCOVERY_MODE PRE_TEST)
gtest_discover_tests(test_backup_engine DISCOVERY_MODE PRE_TEST)
gtest_discover_tests(test_backup_scheduler DISCOVERY_MODE PRE_TEST)
gtest_discover_tests(test_realtime_backup DISCOVERY_MODE PRE_TEST)

# 条件添加Zlib测试
if(ZLIB_TEST_AVAILABLE)
    gtest_discover_tests(test_zlib_compression DISCOVERY_MODE PRE_TEST)
endif()

# 方式 2：手动添加测试
add_test(NAME FrameworkTest COMMAND test_framework_basic --gtest_filter=FrameworkTest.*)
add_test(NAME FileInfoTest COMMAND test_framework_basic --gtest_filter=FileInfoTest.*)
add_test(NAME FileSystemTest COMMAND test_framework_basic --gtest_filter=FileSystemTest.*)
add_test(NAME FileScannerTest COMMAND test_filescanner --gtest_filter=FileScannerTest.*)
add_test(NAME MetadataManagerTest COMMAND test_metadatamanager --gtest_filter=MetadataManagerTest.*)
add_test(NAME PackagerTest COMMAND test_packager --gtest_filter=PackagerTest.*)
add_test(NAME FilterTest COMMAND test_filters --gtest_filter=FilterTest.*)
add_test(NAME IntegrationTest COMMAND test_integration --gtest_filter=IntegrationTest.*)

# 后续添加模块测试（在对应模块实现后添加）
# add_executable(test_metadata test_metadata.cpp)
# target_link_libraries(test_metadata ${TEST_LIBRARIES})
# gtest_discover_tests(test_metadata)