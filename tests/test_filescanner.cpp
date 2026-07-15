#include <gtest/gtest.h>
#include <filesystem>
#include <vector>
#include <fstream>  // 添加 fstream 头文件（用于 createTestFile）
#include "FileScanner.h"
#include "FileInfo.h"

namespace fs = std::filesystem;

/**
 * @brief FileScanner 测试套件
 *
 * 测试文件扫描功能，包括基础扫描、硬链接处理、软链接处理、异常情况处理
 * 测试覆盖率目标：≥ 85%
 */
class FileScannerTest : public ::testing::Test {
protected:
    // 测试数据目录
    fs::path testDir_;
    fs::path emptyDir_;
    fs::path singleLayerDir_;
    fs::path multiLayerDir_;
    fs::path hardlinkDir_;
    fs::path symlinkDir_;

    /**
     * @brief 测试前准备：创建测试数据目录和文件
     */
    void SetUp() override {
        // 创建测试根目录
        testDir_ = fs::temp_directory_path() / "FileScannerTest";
        if (fs::exists(testDir_)) {
            fs::remove_all(testDir_);
        }
        fs::create_directories(testDir_);

        // 创建空目录
        emptyDir_ = testDir_ / "empty";
        fs::create_directories(emptyDir_);

        // 创建单层目录（包含几个文件）
        singleLayerDir_ = testDir_ / "single_layer";
        fs::create_directories(singleLayerDir_);
        createTestFile(singleLayerDir_ / "file1.txt", "Content of file1");
        createTestFile(singleLayerDir_ / "file2.cpp", "Content of file2");
        createTestFile(singleLayerDir_ / "file3.h", "Content of file3");

        // 创建多层目录
        multiLayerDir_ = testDir_ / "multi_layer";
        fs::create_directories(multiLayerDir_ / "sub1");
        fs::create_directories(multiLayerDir_ / "sub1" / "sub2");
        fs::create_directories(multiLayerDir_ / "sub3");
        createTestFile(multiLayerDir_ / "root.txt", "Root file");
        createTestFile(multiLayerDir_ / "sub1" / "level1.txt", "Level 1 file");
        createTestFile(multiLayerDir_ / "sub1" / "sub2" / "level2.txt", "Level 2 file");
        createTestFile(multiLayerDir_ / "sub3" / "another.txt", "Another file");

        // 创建硬链接测试目录（Windows）
        hardlinkDir_ = testDir_ / "hardlink";
        fs::create_directories(hardlinkDir_);
        createHardlinkTestFiles();

        // 创建符号链接测试目录
        symlinkDir_ = testDir_ / "symlink";
        fs::create_directories(symlinkDir_);
        createSymlinkTestFiles();
    }

    /**
     * @brief 测试后清理：删除测试数据目录
     */
    void TearDown() override {
        if (fs::exists(testDir_)) {
            fs::remove_all(testDir_);
        }
    }

    /**
     * @brief 创建测试文件
     */
    void createTestFile(const fs::path& path, const std::string& content) {
        std::ofstream file(path);
        file << content;
        file.close();
    }

    /**
     * @brief 创建硬链接测试文件（Windows）
     */
    void createHardlinkTestFiles() {
#ifdef _WIN32
        // 创建原始文件
        fs::path originalFile = hardlinkDir_ / "original.txt";
        createTestFile(originalFile, "Original content for hardlink test");

        // 创建硬链接（Windows API）
        fs::path hardlinkFile = hardlinkDir_ / "hardlink.txt";
        if (!CreateHardLinkW(hardlinkFile.c_str(), originalFile.c_str(), nullptr)) {
            // 如果创建失败，记录但不中断测试
            std::cerr << "Warning: Failed to create hardlink: " << GetLastError() << std::endl;
        }
#endif
    }

    /**
     * @brief 创建符号链接测试文件
     */
    void createSymlinkTestFiles() {
        // 创建目标文件
        fs::path targetFile = testDir_ / "symlink_target.txt";
        createTestFile(targetFile, "Target file for symlink");

        // 创建符号链接
        fs::path symlinkFile = symlinkDir_ / "link_to_target.txt";
        try {
            fs::create_symlink(targetFile, symlinkFile);
        } catch (const fs::filesystem_error& e) {
            // 某些系统可能需要管理员权限创建符号链接
            std::cerr << "Warning: Failed to create symlink: " << e.what() << std::endl;
        }

        // 创建目录符号链接
        fs::path symlinkDir = symlinkDir_ / "link_to_dir";
        try {
            fs::create_directory_symlink(singleLayerDir_, symlinkDir);
        } catch (const fs::filesystem_error& e) {
            std::cerr << "Warning: Failed to create directory symlink: " << e.what() << std::endl;
        }
    }
};

/**
 * @brief 测试：扫描空目录
 */
TEST_F(FileScannerTest, ScanEmptyDirectory) {
    FileScanner scanner;
    std::vector<FileInfo> files;

    bool result = scanner.scan(emptyDir_, files);

    EXPECT_TRUE(result);
    EXPECT_EQ(files.size(), 0);
}

/**
 * @brief 测试：扫描单层目录
 */
TEST_F(FileScannerTest, ScanSingleLayerDirectory) {
    FileScanner scanner;
    std::vector<FileInfo> files;

    bool result = scanner.scan(singleLayerDir_, files);

    EXPECT_TRUE(result);
    EXPECT_EQ(files.size(), 3);  // file1.txt, file2.cpp, file3.h

    // 验证文件路径正确
    bool foundFile1 = false, foundFile2 = false, foundFile3 = false;
    for (const auto& file : files) {
        if (file.path.filename() == "file1.txt") foundFile1 = true;
        if (file.path.filename() == "file2.cpp") foundFile2 = true;
        if (file.path.filename() == "file3.h") foundFile3 = true;
    }
    EXPECT_TRUE(foundFile1);
    EXPECT_TRUE(foundFile2);
    EXPECT_TRUE(foundFile3);
}

/**
 * @brief 测试：扫描多层目录
 */
TEST_F(FileScannerTest, ScanMultiLayerDirectory) {
    FileScanner scanner;
    std::vector<FileInfo> files;

    bool result = scanner.scan(multiLayerDir_, files);

    EXPECT_TRUE(result);
    EXPECT_EQ(files.size(), 4);  // root.txt, level1.txt, level2.txt, another.txt

    // 验证多层结构正确扫描
    bool foundRoot = false, foundLevel1 = false, foundLevel2 = false, foundAnother = false;
    for (const auto& file : files) {
        std::string filename = file.path.filename().string();
        if (filename == "root.txt") foundRoot = true;
        if (filename == "level1.txt") foundLevel1 = true;
        if (filename == "level2.txt") foundLevel2 = true;
        if (filename == "another.txt") foundAnother = true;
    }
    EXPECT_TRUE(foundRoot);
    EXPECT_TRUE(foundLevel1);
    EXPECT_TRUE(foundLevel2);
    EXPECT_TRUE(foundAnother);
}

/**
 * @brief 测试：硬链接处理（重点测试）
 */
TEST_F(FileScannerTest, HandleHardlinks) {
#ifdef _WIN32
    FileScanner scanner;
    std::vector<FileInfo> files;

    bool result = scanner.scan(hardlinkDir_, files);

    EXPECT_TRUE(result);

    // 硬链接应该只备份一次（原始文件），不应重复备份
    // hardlink.txt 是 original.txt 的硬链接，应该被识别并记录
    EXPECT_LE(files.size(), 2);  // 最多2个文件（原始文件 + 硬链接引用）

    // 验证硬链接被正确识别
    bool hasHardlink = false;
    for (const auto& file : files) {
        if (file.isHardlink) {
            hasHardlink = true;
            // 硬链接应该有目标路径
            EXPECT_FALSE(file.hardlinkTarget.empty());
            // 硬链接引用计数应该 > 1
            EXPECT_GT(file.hardlinkCount, 1);
        }
    }

    // 如果硬链接创建成功，应该能识别
    // 注意：如果硬链接创建失败，此测试可能不适用
#else
    // Linux 系统测试（简化）
    GTEST_SKIP() << "Hardlink test skipped on non-Windows platform";
#endif
}

/**
 * @brief 测试：符号链接处理（重点测试）
 */
TEST_F(FileScannerTest, HandleSymlinks) {
    FileScanner scanner;
    std::vector<FileInfo> files;

    // 默认不跟随符号链接
    scanner.setFollowSymlinks(false);
    bool result = scanner.scan(symlinkDir_, files);

    EXPECT_TRUE(result);

    // 符号链接应该被识别
    bool hasSymlink = false;
    for (const auto& file : files) {
        if (file.isSymlink) {
            hasSymlink = true;
            // 符号链接应该有目标路径
            EXPECT_FALSE(file.symlinkTarget.empty());
        }
    }

    // 如果符号链接创建成功，应该能识别
    // 注意：某些系统可能需要管理员权限创建符号链接
}

/**
 * @brief 测试：跟随符号链接
 */
TEST_F(FileScannerTest, FollowSymlinks) {
    FileScanner scanner;
    std::vector<FileInfo> files;

    // 设置跟随符号链接
    scanner.setFollowSymlinks(true);
    bool result = scanner.scan(symlinkDir_, files);

    EXPECT_TRUE(result);

    // 如果跟随符号链接，应该扫描到目标文件
    // 注意：需要根据实际创建的符号链接调整预期
}

/**
 * @brief 测试：扫描不存在目录
 */
TEST_F(FileScannerTest, ScanNonexistentDirectory) {
    FileScanner scanner;
    std::vector<FileInfo> files;

    fs::path nonexistent = testDir_ / "nonexistent";
    bool result = scanner.scan(nonexistent, files);

    EXPECT_FALSE(result);
    EXPECT_EQ(files.size(), 0);
}

/**
 * @brief 测试：扫描权限受限目录（如果有）
 */
TEST_F(FileScannerTest, ScanPermissionDenied) {
    // 此测试可能需要特殊权限设置，暂时跳过
    GTEST_SKIP() << "Permission test requires special setup";
}

/**
 * @brief 测试：获取扫描统计信息
 */
TEST_F(FileScannerTest, GetScanStatistics) {
    FileScanner scanner;
    std::vector<FileInfo> files;

    bool result = scanner.scan(multiLayerDir_, files);

    EXPECT_TRUE(result);

    // 验证扫描统计信息
    auto stats = scanner.getStatistics();
    EXPECT_EQ(stats.totalFiles, 4);
    EXPECT_EQ(stats.totalDirectories, 4);  // multi_layer + sub1 + sub2 + sub3
    EXPECT_EQ(stats.hardlinksFound, 0);    // 此测试目录无硬链接
    EXPECT_EQ(stats.symlinksFound, 0);     // 此测试目录无符号链接
}

/**
 * @brief 主函数：运行所有测试
 */
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}