// 集成测试程序 - 验证备份还原功能完整流程
// 使用临时目录测试，不影响真实文件

#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include "core/FileInfo.h"
#include "core/FileScanner.h"
#include "core/MetadataManager.h"
#include "pack/Packager.h"
#include "filter/FilterChain.h"
#include "filter/TypeFilter.h"

namespace fs = std::filesystem;

class IntegrationTest : public ::testing::Test {
protected:
    fs::path testDir_;
    fs::path restoreDir_;
    fs::path packageFile_;

    void SetUp() override {
        testDir_ = fs::temp_directory_path() / "BackupTest_Source";
        restoreDir_ = fs::temp_directory_path() / "BackupTest_Restore";
        packageFile_ = fs::temp_directory_path() / "backup.pkg";

        fs::remove_all(testDir_);
        fs::remove_all(restoreDir_);
        fs::remove(packageFile_);

        fs::create_directories(testDir_);
        fs::create_directories(testDir_ / "subdir");

        // 创建测试文件
        std::ofstream(testDir_ / "file1.txt") << "Test content 1";
        std::ofstream(testDir_ / "file2.pdf") << "Test content 2";
        std::ofstream(testDir_ / "subdir" / "file3.txt") << "Test content 3";
    }

    void TearDown() override {
        std::cout << "\n测试文件位于：" << testDir_ << std::endl;
        std::cout << "还原文件位于：" << restoreDir_ << std::endl;
    }
};

// 测试完整备份还原
TEST_F(IntegrationTest, FullBackupRestore) {
    std::cout << "\n=== 测试完整备份还原流程 ===\n";

    // 1. 扫描
    FileScanner scanner;
    std::vector<FileInfo> files;
    scanner.scan(testDir_, files);
    std::cout << "扫描到 " << files.size() << " 个文件\n";

    // 2. 提取元数据
    MetadataManager meta;
    std::vector<FileInfo> metaFiles;
    for (auto& f : files) {
        FileInfo info = meta.extractMetadata(f.path);
        info.path = f.path;  // 保持原始路径
        metaFiles.push_back(info);
    }

    // 3. 打包
    Packager packager;
    bool packed = packager.pack(metaFiles, packageFile_, nullptr, testDir_);
    ASSERT_TRUE(packed);
    std::cout << "打包完成，大小: " << fs::file_size(packageFile_) << " 字节\n";

    // 4. 解包
    bool unpacked = packager.unpack(packageFile_, restoreDir_);
    ASSERT_TRUE(unpacked);
    std::cout << "解包完成\n";

    // 5. 验证
    int matched = 0;
    for (auto& f : files) {
        if (!f.isDirectory) {
            fs::path rel = fs::relative(f.path, testDir_);
            fs::path restored = restoreDir_ / rel;
            if (fs::exists(restored) && fs::file_size(f.path) == fs::file_size(restored)) {
                matched++;
            }
        }
    }
    ASSERT_EQ(matched, scanner.getStatistics().totalFiles);
    std::cout << "验证通过，匹配文件: " << matched << "\n";
}

// 测试过滤备份
TEST_F(IntegrationTest, FilteredBackup) {
    std::cout << "\n=== 测试带过滤器的备份 ===\n";

    FileScanner scanner;
    std::vector<FileInfo> files;
    scanner.scan(testDir_, files);

    FilterChain chain(FilterChain::Mode::AND);
    TypeFilter typeFilter(std::vector<std::string>{"txt"}, std::vector<std::string>{});
    chain.addFilter(std::make_shared<TypeFilter>(typeFilter));

    std::vector<FileInfo> filtered;
    for (auto& f : files) {
        if (chain.accept(f)) filtered.push_back(f);
    }
    std::cout << "过滤后保留 " << filtered.size() << " 个txt文件\n";

    MetadataManager meta;
    std::vector<FileInfo> metaFiles;
    for (auto& f : filtered) {
        FileInfo info = meta.extractMetadata(f.path);
        info.path = f.path;
        metaFiles.push_back(info);
    }

    Packager packager;
    packager.pack(metaFiles, packageFile_, nullptr, testDir_);
    packager.unpack(packageFile_, restoreDir_);

    int txtCount = 0;
    for (auto& e : fs::recursive_directory_iterator(restoreDir_)) {
        if (e.path().extension() == ".txt") txtCount++;
    }
    ASSERT_EQ(txtCount, filtered.size());
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}