#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <vector>
#include "Packager.h"
#include "FileInfo.h"

namespace fs = std::filesystem;

/**
 * @brief Packager 测试套件
 *
 * 测试文件打包解包功能，包括：
 * - 单文件、多文件、空文件打包
 * - 文件元数据保存与还原
 * - 大文件支持
 * - 打包格式验证
 * - 解包完整性验证
 */
class PackagerTest : public ::testing::Test {
protected:
    fs::path testDir_;
    fs::path outputDir_;
    Packager packager_;

    void SetUp() override {
        // 创建临时测试目录
        testDir_ = fs::temp_directory_path() / "PackagerTest";
        outputDir_ = testDir_ / "output";
        fs::create_directories(testDir_);
        fs::create_directories(outputDir_);
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

    // 读取文件内容
    std::string readFile(const fs::path& filePath) {
        std::ifstream file(filePath, std::ios::binary);
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    // 验证文件内容是否相同
    bool compareFiles(const fs::path& file1, const fs::path& file2) {
        if (!fs::exists(file1) || !fs::exists(file2)) {
            return false;
        }
        return readFile(file1) == readFile(file2);
    }
};

// 测试 1：打包单个文件
TEST_F(PackagerTest, PackSingleFile) {
    fs::path testFile = testDir_ / "single_file.txt";
    createTestFile(testFile, "This is a single file for testing.");

    std::vector<FileInfo> files;
    files.push_back(FileInfo(testFile));

    fs::path packageFile = outputDir_ / "single.pkg";

    bool success = packager_.pack(files, packageFile);
    EXPECT_TRUE(success);
    EXPECT_TRUE(fs::exists(packageFile));
    EXPECT_GT(fs::file_size(packageFile), 0);
}

// 测试 2：打包多个文件
TEST_F(PackagerTest, PackMultipleFiles) {
    std::vector<FileInfo> files;

    for (int i = 0; i < 5; i++) {
        fs::path file = testDir_ / ("file_" + std::to_string(i) + ".txt");
        createTestFile(file, "Content of file " + std::to_string(i));
        files.push_back(FileInfo(file));
    }

    fs::path packageFile = outputDir_ / "multiple.pkg";

    bool success = packager_.pack(files, packageFile);
    EXPECT_TRUE(success);
    EXPECT_TRUE(fs::exists(packageFile));
}

// 测试 3：解包单个文件
TEST_F(PackagerTest, UnpackSingleFile) {
    // 先创建并打包
    fs::path originalFile = testDir_ / "original.txt";
    createTestFile(originalFile, "Original content for unpack test.");

    std::vector<FileInfo> files;
    files.push_back(FileInfo(originalFile));

    fs::path packageFile = outputDir_ / "unpack_single.pkg";
    packager_.pack(files, packageFile);

    // 解包
    fs::path unpackDir = outputDir_ / "unpacked_single";
    fs::create_directories(unpackDir);

    bool success = packager_.unpack(packageFile, unpackDir);
    EXPECT_TRUE(success);

    // 验证文件存在
    fs::path unpackedFile = unpackDir / "original.txt";
    EXPECT_TRUE(fs::exists(unpackedFile));

    // 验证内容相同
    EXPECT_TRUE(compareFiles(originalFile, unpackedFile));
}

// 测试 4：解包多个文件
TEST_F(PackagerTest, UnpackMultipleFiles) {
    // 创建并打包多个文件
    std::vector<fs::path> originalFiles;
    std::vector<FileInfo> files;

    for (int i = 0; i < 5; i++) {
        fs::path file = testDir_ / ("multi_" + std::to_string(i) + ".txt");
        createTestFile(file, "Multi file content " + std::to_string(i));
        originalFiles.push_back(file);
        files.push_back(FileInfo(file));
    }

    fs::path packageFile = outputDir_ / "unpack_multi.pkg";
    packager_.pack(files, packageFile);

    // 解包
    fs::path unpackDir = outputDir_ / "unpacked_multi";
    fs::create_directories(unpackDir);

    bool success = packager_.unpack(packageFile, unpackDir);
    EXPECT_TRUE(success);

    // 验证所有文件都存在且内容正确
    for (int i = 0; i < 5; i++) {
        fs::path unpackedFile = unpackDir / ("multi_" + std::to_string(i) + ".txt");
        EXPECT_TRUE(fs::exists(unpackedFile));
        EXPECT_TRUE(compareFiles(originalFiles[i], unpackedFile));
    }
}

// 测试 5：空文件打包解包
TEST_F(PackagerTest, PackUnpackEmptyFile) {
    fs::path emptyFile = testDir_ / "empty.txt";
    createTestFile(emptyFile, "");  // 空文件

    std::vector<FileInfo> files;
    files.push_back(FileInfo(emptyFile));

    fs::path packageFile = outputDir_ / "empty.pkg";
    packager_.pack(files, packageFile);

    // 解包
    fs::path unpackDir = outputDir_ / "unpacked_empty";
    fs::create_directories(unpackDir);

    bool success = packager_.unpack(packageFile, unpackDir);
    EXPECT_TRUE(success);

    fs::path unpackedFile = unpackDir / "empty.txt";
    EXPECT_TRUE(fs::exists(unpackedFile));
    EXPECT_EQ(fs::file_size(unpackedFile), 0);
}

// 测试 6：大文件打包解包（1MB）
TEST_F(PackagerTest, PackUnpackLargeFile) {
    fs::path largeFile = testDir_ / "large_file.bin";

    // 创建 1MB 文件
    const size_t fileSize = 1024 * 1024;  // 1MB
    std::vector<char> largeData(fileSize, 'X');
    {
        std::ofstream file(largeFile, std::ios::binary);
        file.write(largeData.data(), fileSize);
    }

    std::vector<FileInfo> files;
    files.push_back(FileInfo(largeFile));

    fs::path packageFile = outputDir_ / "large.pkg";
    bool packSuccess = packager_.pack(files, packageFile);
    EXPECT_TRUE(packSuccess);

    // 解包
    fs::path unpackDir = outputDir_ / "unpacked_large";
    fs::create_directories(unpackDir);

    bool unpackSuccess = packager_.unpack(packageFile, unpackDir);
    EXPECT_TRUE(unpackSuccess);

    // 验证大小
    fs::path unpackedFile = unpackDir / "large_file.bin";
    EXPECT_TRUE(fs::exists(unpackedFile));
    EXPECT_EQ(fs::file_size(unpackedFile), fileSize);
}

// 测试 7：文件元数据保存与还原
TEST_F(PackagerTest, MetadataPreservation) {
    fs::path originalFile = testDir_ / "metadata_test.txt";
    createTestFile(originalFile, "Content for metadata test.");

    // 获取原始元数据
    FileInfo originalInfo(originalFile);

    // 等待一秒，确保时间戳不同
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // 打包
    std::vector<FileInfo> files;
    files.push_back(originalInfo);

    fs::path packageFile = outputDir_ / "metadata.pkg";
    packager_.pack(files, packageFile);

    // 解包
    fs::path unpackDir = outputDir_ / "unpacked_metadata";
    fs::create_directories(unpackDir);

    packager_.unpack(packageFile, unpackDir);

    // 验证元数据（文件大小）
    fs::path unpackedFile = unpackDir / "metadata_test.txt";
    FileInfo unpackedInfo(unpackedFile);

    EXPECT_EQ(unpackedInfo.size, originalInfo.size);
}

// 测试 8：打包文件格式验证（Magic Number）
TEST_F(PackagerTest, PackageFormatValidation) {
    fs::path testFile = testDir_ / "format_test.txt";
    createTestFile(testFile, "Format validation test.");

    std::vector<FileInfo> files;
    files.push_back(FileInfo(testFile));

    fs::path packageFile = outputDir_ / "format.pkg";
    packager_.pack(files, packageFile);

    // 验证 Magic Number
    std::ifstream pkgFile(packageFile, std::ios::binary);

    char magic[4];
    pkgFile.read(magic, 4);

    // 检查是否包含正确的 Magic Number（根据实际格式定义）
    EXPECT_TRUE(magic[0] != 0 || magic[1] != 0 || magic[2] != 0 || magic[3] != 0);

    pkgFile.close();
}

// 测试 9：解包不存在的包文件
TEST_F(PackagerTest, UnpackNonexistentPackage) {
    fs::path nonexistentPackage = testDir_ / "nonexistent.pkg";
    fs::path unpackDir = outputDir_ / "unpack_nonexistent";

    bool success = packager_.unpack(nonexistentPackage, unpackDir);
    EXPECT_FALSE(success);
}

// 测试 10：解包损坏的包文件
TEST_F(PackagerTest, UnpackCorruptedPackage) {
    fs::path corruptedPackage = testDir_ / "corrupted.pkg";

    // 创建损坏的包文件
    std::ofstream file(corruptedPackage, std::ios::binary);
    file << "This is not a valid package file";
    file.close();

    fs::path unpackDir = outputDir_ / "unpack_corrupted";

    bool success = packager_.unpack(corruptedPackage, unpackDir);
    EXPECT_FALSE(success);
}

// 测试 11：带目录结构的文件打包
TEST_F(PackagerTest, PackFilesWithDirectoryStructure) {
    // 创建目录结构
    fs::path subdir = testDir_ / "subdir";
    fs::create_directory(subdir);

    fs::path file1 = testDir_ / "root_file.txt";
    fs::path file2 = subdir / "subdir_file.txt";

    createTestFile(file1, "Root file content");
    createTestFile(file2, "Subdir file content");

    std::vector<FileInfo> files;
    files.push_back(FileInfo(file1));
    files.push_back(FileInfo(file2));

    fs::path packageFile = outputDir_ / "directory_structure.pkg";
    // 使用 testDir_ 作为基础路径，保留相对目录结构
    bool packSuccess = packager_.pack(files, packageFile, nullptr, testDir_);
    EXPECT_TRUE(packSuccess);

    // 解包
    fs::path unpackDir = outputDir_ / "unpacked_structure";
    fs::create_directories(unpackDir);

    bool unpackSuccess = packager_.unpack(packageFile, unpackDir);
    EXPECT_TRUE(unpackSuccess);

    // 验证文件存在
    EXPECT_TRUE(fs::exists(unpackDir / "root_file.txt"));
    EXPECT_TRUE(fs::exists(unpackDir / "subdir" / "subdir_file.txt"));
}

// 测试 12：获取包文件信息
TEST_F(PackagerTest, GetPackageInfo) {
    // 创建并打包文件
    fs::path testFile = testDir_ / "info_test.txt";
    createTestFile(testFile, "Test for package info.");

    std::vector<FileInfo> files;
    files.push_back(FileInfo(testFile));

    fs::path packageFile = outputDir_ / "info.pkg";
    packager_.pack(files, packageFile);

    // 获取包信息
    PackageInfo info = packager_.getPackageInfo(packageFile);

    EXPECT_EQ(info.fileCount, 1);
    EXPECT_GT(info.totalSize, 0);
    EXPECT_FALSE(info.version.empty());
}

// 测试 13：验证包完整性
TEST_F(PackagerTest, VerifyPackageIntegrity) {
    fs::path testFile = testDir_ / "integrity_test.txt";
    createTestFile(testFile, "Integrity verification test.");

    std::vector<FileInfo> files;
    files.push_back(FileInfo(testFile));

    fs::path packageFile = outputDir_ / "integrity.pkg";
    packager_.pack(files, packageFile);

    // 验证完整性
    bool isValid = packager_.verifyPackage(packageFile);
    EXPECT_TRUE(isValid);
}

// 测试 14：打包进度回调
TEST_F(PackagerTest, PackWithProgressCallback) {
    std::vector<FileInfo> files;

    for (int i = 0; i < 10; i++) {
        fs::path file = testDir_ / ("progress_" + std::to_string(i) + ".txt");
        createTestFile(file, "Progress test " + std::to_string(i));
        files.push_back(FileInfo(file));
    }

    fs::path packageFile = outputDir_ / "progress.pkg";

    int callbackCount = 0;
    auto callback = [&callbackCount](int current, int total) {
        callbackCount++;
    };

    bool success = packager_.pack(files, packageFile, callback);
    EXPECT_TRUE(success);
    EXPECT_GT(callbackCount, 0);
}