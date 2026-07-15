#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <chrono>
#include "MetadataManager.h"
#include "FileInfo.h"

namespace fs = std::filesystem;

/**
 * @brief MetadataManager 测试套件
 *
 * 测试 Windows 文件元数据的提取、保存和恢复功能
 */
class MetadataManagerTest : public ::testing::Test {
protected:
    fs::path testDir_;
    MetadataManager manager_;

    void SetUp() override {
        // 创建临时测试目录
        testDir_ = fs::temp_directory_path() / "MetadataManagerTest";
        fs::create_directories(testDir_);
    }

    void TearDown() override {
        // 清理测试目录
        if (fs::exists(testDir_)) {
            fs::remove_all(testDir_);
        }
    }

    // 创建测试文件
    void createTestFile(const fs::path& filePath, const std::string& content = "test content") {
        std::ofstream file(filePath, std::ios::binary);
        file << content;
        file.close();
    }
};

// 测试 1：提取普通文件的元数据
TEST_F(MetadataManagerTest, ExtractBasicMetadata) {
    fs::path testFile = testDir_ / "basic_file.txt";
    createTestFile(testFile);

    FileInfo info = manager_.extractMetadata(testFile);

    EXPECT_EQ(info.path, testFile);
    EXPECT_GT(info.size, 0);
    EXPECT_FALSE(info.isDirectory);
    EXPECT_FALSE(info.isHardlink);
    EXPECT_FALSE(info.isSymlink);
    EXPECT_TRUE(info.creationTime > 0);
    EXPECT_TRUE(info.lastWriteTime > 0);
    EXPECT_TRUE(info.lastAccessTime > 0);
}

// 测试 2：提取目录的元数据
TEST_F(MetadataManagerTest, ExtractDirectoryMetadata) {
    fs::path testSubDir = testDir_ / "subdir";
    fs::create_directory(testSubDir);

    FileInfo info = manager_.extractMetadata(testSubDir);

    EXPECT_EQ(info.path, testSubDir);
    EXPECT_EQ(info.size, 0);
    EXPECT_TRUE(info.isDirectory);
    EXPECT_FALSE(info.isHardlink);
    EXPECT_FALSE(info.isSymlink);
}

// 测试 3：设置文件时间戳
TEST_F(MetadataManagerTest, SetFileTimestamps) {
    fs::path testFile = testDir_ / "timestamp_file.txt";
    createTestFile(testFile);

    // 定义新的时间戳（2024年1月1日）
    uint64_t newCreationTime = 1704067200ULL * 10000000ULL + 116444736000000000ULL;
    uint64_t newWriteTime = 1704070800ULL * 10000000ULL + 116444736000000000ULL;
    uint64_t newAccessTime = 1704074400ULL * 10000000ULL + 116444736000000000ULL;

    FileInfo info;
    info.path = testFile;
    info.creationTime = newCreationTime;
    info.lastWriteTime = newWriteTime;
    info.lastAccessTime = newAccessTime;

    bool success = manager_.setMetadata(info);
    EXPECT_TRUE(success);

    // 验证时间戳是否正确设置
    FileInfo verifyInfo = manager_.extractMetadata(testFile);

    // 允许 2 秒误差（文件系统时间精度）
    uint64_t tolerance = 20000000ULL;  // 2 秒（以 100ns 为单位）

    EXPECT_NEAR(verifyInfo.creationTime, newCreationTime, tolerance);
    EXPECT_NEAR(verifyInfo.lastWriteTime, newWriteTime, tolerance);
    EXPECT_NEAR(verifyInfo.lastAccessTime, newAccessTime, tolerance);
}

// 测试 4：设置文件属性
TEST_F(MetadataManagerTest, SetFileAttributes) {
    fs::path testFile = testDir_ / "readonly_file.txt";
    createTestFile(testFile);

    FileInfo info;
    info.path = testFile;
    info.attributes = FILE_ATTRIBUTE_READONLY;

    bool success = manager_.setMetadata(info);
    EXPECT_TRUE(success);

    // 验证文件是否为只读
    DWORD attrs = GetFileAttributesW(testFile.wstring().c_str());
    EXPECT_TRUE(attrs & FILE_ATTRIBUTE_READONLY);

    // 清除只读属性以便删除
    SetFileAttributesW(testFile.wstring().c_str(), FILE_ATTRIBUTE_NORMAL);
}

// 测试 5：设置隐藏属性
TEST_F(MetadataManagerTest, SetHiddenAttribute) {
    fs::path testFile = testDir_ / "hidden_file.txt";
    createTestFile(testFile);

    FileInfo info;
    info.path = testFile;
    info.attributes = FILE_ATTRIBUTE_HIDDEN;

    bool success = manager_.setMetadata(info);
    EXPECT_TRUE(success);

    // 验证文件是否为隐藏
    DWORD attrs = GetFileAttributesW(testFile.wstring().c_str());
    EXPECT_TRUE(attrs & FILE_ATTRIBUTE_HIDDEN);

    // 清除隐藏属性
    SetFileAttributesW(testFile.wstring().c_str(), FILE_ATTRIBUTE_NORMAL);
}

// 测试 6：提取不存在的文件的元数据
TEST_F(MetadataManagerTest, ExtractNonexistentFileMetadata) {
    fs::path nonexistentFile = testDir_ / "nonexistent.txt";

    FileInfo info = manager_.extractMetadata(nonexistentFile);

    EXPECT_TRUE(info.path.empty());
    EXPECT_EQ(info.size, 0);
}

// 测试 7：设置不存在文件的时间戳
TEST_F(MetadataManagerTest, SetNonexistentFileTimestamps) {
    fs::path nonexistentFile = testDir_ / "nonexistent_timestamp.txt";

    FileInfo info;
    info.path = nonexistentFile;
    info.creationTime = 0;
    info.lastWriteTime = 0;

    bool success = manager_.setMetadata(info);
    EXPECT_FALSE(success);
}

// 测试 8：批量提取元数据
TEST_F(MetadataManagerTest, ExtractBatchMetadata) {
    std::vector<fs::path> testFiles;

    for (int i = 0; i < 10; i++) {
        fs::path file = testDir_ / ("batch_file_" + std::to_string(i) + ".txt");
        createTestFile(file, "content " + std::to_string(i));
        testFiles.push_back(file);
    }

    std::vector<FileInfo> infos = manager_.extractBatchMetadata(testFiles);

    EXPECT_EQ(infos.size(), 10);

    for (const auto& info : infos) {
        EXPECT_FALSE(info.path.empty());
        EXPECT_GT(info.size, 0);
        EXPECT_TRUE(info.creationTime > 0);
        EXPECT_TRUE(info.lastWriteTime > 0);
    }
}

// 测试 9：验证元数据完整性
TEST_F(MetadataManagerTest, MetadataIntegrity) {
    fs::path originalFile = testDir_ / "original.txt";
    fs::path copiedFile = testDir_ / "copied.txt";

    createTestFile(originalFile, "original content");

    FileInfo originalInfo = manager_.extractMetadata(originalFile);

    // 复制文件
    fs::copy_file(originalFile, copiedFile);

    // 设置复制的文件与原文件相同的元数据
    bool success = manager_.setMetadata(originalInfo, copiedFile);
    EXPECT_TRUE(success);

    // 验证元数据是否正确复制
    FileInfo copiedInfo = manager_.extractMetadata(copiedFile);

    // 允许时间戳有轻微差异（复制操作本身会修改时间）
    uint64_t tolerance = 50000000ULL;  // 5 秒

    EXPECT_NEAR(copiedInfo.creationTime, originalInfo.creationTime, tolerance);
    EXPECT_NEAR(copiedInfo.lastWriteTime, originalInfo.lastWriteTime, tolerance);
    EXPECT_EQ(copiedInfo.attributes, originalInfo.attributes);
}

// 测试 10：元数据序列化到字符串
TEST_F(MetadataManagerTest, MetadataSerialization) {
    fs::path testFile = testDir_ / "serialize_file.txt";
    createTestFile(testFile);

    FileInfo info = manager_.extractMetadata(testFile);

    std::string serialized = manager_.serializeMetadata(info);

    EXPECT_FALSE(serialized.empty());

    // 反序列化
    FileInfo deserialized = manager_.deserializeMetadata(serialized);

    EXPECT_EQ(deserialized.path, info.path);
    EXPECT_EQ(deserialized.size, info.size);
    EXPECT_EQ(deserialized.isDirectory, info.isDirectory);
    EXPECT_EQ(deserialized.creationTime, info.creationTime);
    EXPECT_EQ(deserialized.lastWriteTime, info.lastWriteTime);
    EXPECT_EQ(deserialized.lastAccessTime, info.lastAccessTime);
    EXPECT_EQ(deserialized.attributes, info.attributes);
}

// 测试 11：元数据持久化到文件
TEST_F(MetadataManagerTest, MetadataPersistence) {
    fs::path testFile = testDir_ / "persist_file.txt";
    fs::path metadataFile = testDir_ / "metadata.json";

    createTestFile(testFile);

    FileInfo info = manager_.extractMetadata(testFile);

    // 持久化元数据
    bool success = manager_.saveMetadataToFile(info, metadataFile);
    EXPECT_TRUE(success);
    EXPECT_TRUE(fs::exists(metadataFile));

    // 从文件加载元数据
    FileInfo loaded = manager_.loadMetadataFromFile(metadataFile);

    EXPECT_EQ(loaded.path, info.path);
    EXPECT_EQ(loaded.size, info.size);
    EXPECT_EQ(loaded.isDirectory, info.isDirectory);
    EXPECT_EQ(loaded.creationTime, info.creationTime);
    EXPECT_EQ(loaded.lastWriteTime, info.lastWriteTime);
    EXPECT_EQ(loaded.lastAccessTime, info.lastAccessTime);
    EXPECT_EQ(loaded.attributes, info.attributes);
}

// 测试 12：获取文件权限信息
TEST_F(MetadataManagerTest, GetFilePermissions) {
    fs::path testFile = testDir_ / "permission_file.txt";
    createTestFile(testFile);

    FileInfo info = manager_.extractMetadata(testFile);

    // Windows 下权限信息
    EXPECT_FALSE(info.permissions.empty());
    EXPECT_TRUE(info.permissions.find("read") != std::string::npos ||
                info.permissions.find("write") != std::string::npos);
}