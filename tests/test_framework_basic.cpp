#include <gtest/gtest.h>
#include <filesystem>
#include "FileInfo.h"

namespace fs = std::filesystem;

/**
 * @brief 测试框架基础测试
 *
 * 验证 Google Test 框架是否正确配置和运行
 */
TEST(FrameworkTest, TestFrameworkWorks) {
    // 最简单的测试：验证测试框架可以运行
    EXPECT_EQ(1 + 1, 2);
    EXPECT_TRUE(true);
    EXPECT_FALSE(false);
}

/**
 * @brief 测试 FileInfo 类的基本功能
 */
TEST(FileInfoTest, DefaultConstructor) {
    FileInfo fileInfo;

    EXPECT_FALSE(fileInfo.isHardlink);
    EXPECT_FALSE(fileInfo.isSymlink);
    EXPECT_EQ(fileInfo.size, 0);
    EXPECT_EQ(fileInfo.hardlinkCount, 0);
}

TEST(FileInfoTest, PathConstructor) {
    fs::path testPath = "test_file.txt";
    FileInfo fileInfo(testPath);

    EXPECT_EQ(fileInfo.path, testPath);
}

TEST(FileInfoTest, SetAndGetSize) {
    FileInfo fileInfo;
    uint64_t testSize = 1024;

    fileInfo.size = testSize;
    EXPECT_EQ(fileInfo.size, testSize);
}

TEST(FileInfoTest, SetAndGetHardlink) {
    FileInfo fileInfo;

    fileInfo.isHardlink = true;
    EXPECT_TRUE(fileInfo.isHardlink);

    fs::path targetPath = "original_file.txt";
    fileInfo.hardlinkTarget = targetPath.string();
    EXPECT_EQ(fileInfo.hardlinkTarget, targetPath.string());

    uint32_t count = 3;
    fileInfo.hardlinkCount = count;
    EXPECT_EQ(fileInfo.hardlinkCount, count);
}

TEST(FileInfoTest, SetAndGetSymlink) {
    FileInfo fileInfo;

    fileInfo.isSymlink = true;
    EXPECT_TRUE(fileInfo.isSymlink);

    fs::path targetPath = "target_file.txt";
    fileInfo.symlinkTarget = targetPath;
    EXPECT_EQ(fileInfo.symlinkTarget, targetPath);
}

#ifdef _WIN32
TEST(FileInfoTest, WindowsMetadata) {
    FileInfo fileInfo;

    uint64_t testTime = 12345ULL;

    fileInfo.creationTime = testTime;
    EXPECT_EQ(fileInfo.creationTime, testTime);

    fileInfo.lastWriteTime = testTime;
    EXPECT_EQ(fileInfo.lastWriteTime, testTime);

    DWORD testAttr = FILE_ATTRIBUTE_READONLY;
    fileInfo.attributes = testAttr;
    EXPECT_EQ(fileInfo.attributes, testAttr);
}
#endif

/**
 * @brief 测试文件系统路径操作
 */
TEST(FileSystemTest, PathOperations) {
    fs::path testPath = fs::current_path() / "test_directory";

    // 测试路径拼接
    EXPECT_TRUE(testPath.has_filename());
    EXPECT_TRUE(testPath.has_parent_path());
}

/**
 * @brief 主函数：运行所有测试
 */
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}