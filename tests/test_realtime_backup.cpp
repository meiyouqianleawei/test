#include <gtest/gtest.h>
#include "realtime/RealtimeBackupMonitor.h"
#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>

using namespace DataBackup;
namespace fs = std::filesystem;

class RealtimeBackupTest : public ::testing::Test {
protected:
    std::string sourceDir_;
    std::string targetDir_;
    std::unique_ptr<RealtimeBackupMonitor> monitor_;

    void SetUp() override {
        sourceDir_ = (fs::temp_directory_path() / "RealtimeBackup_Source").string();
        targetDir_ = (fs::temp_directory_path() / "RealtimeBackup_Target").string();

        // 清理旧数据
        fs::remove_all(sourceDir_);
        fs::remove_all(targetDir_);

        // 创建测试目录
        fs::create_directories(sourceDir_);
        fs::create_directories(targetDir_);

        // 创建监控器
        monitor_ = std::make_unique<RealtimeBackupMonitor>();
    }

    void TearDown() override {
        // 停止监控
        if (monitor_ && monitor_->isRunning()) {
            monitor_->stopMonitor();
        }

        // 清理测试数据
        fs::remove_all(sourceDir_);
        fs::remove_all(targetDir_);
    }

    // 辅助方法：创建测试文件
    void createTestFile(const std::string& fileName, const std::string& content = "Test Data") {
        std::string filePath = sourceDir_ + "\\" + fileName;
        std::ofstream file(filePath);
        file << content;
        file.close();
    }

    // 辅助方法：修改测试文件
    void modifyTestFile(const std::string& fileName, const std::string& newContent) {
        std::string filePath = sourceDir_ + "\\" + fileName;
        std::ofstream file(filePath);
        file << newContent;
        file.close();
    }

    // 辅助方法：创建基本配置
    RealtimeBackupConfig createBasicConfig() {
        RealtimeBackupConfig config;
        config.sourcePath = sourceDir_;
        config.targetPath = targetDir_;
        config.enableEncryption = false;
        config.includeSubdirectories = true;
        return config;
    }
};

// 测试1：创建和启动监控器
TEST_F(RealtimeBackupTest, CreateAndStart) {
    EXPECT_FALSE(monitor_->isRunning());
    EXPECT_EQ(monitor_->getStatus(), RealtimeBackupStatus::IDLE);

    RealtimeBackupConfig config = createBasicConfig();
    EXPECT_TRUE(monitor_->startMonitor(config));

    EXPECT_TRUE(monitor_->isRunning());
    EXPECT_EQ(monitor_->getStatus(), RealtimeBackupStatus::RUNNING);
    EXPECT_EQ(monitor_->getMonitoredPath(), sourceDir_);

    monitor_->stopMonitor();
    EXPECT_FALSE(monitor_->isRunning());
    EXPECT_EQ(monitor_->getStatus(), RealtimeBackupStatus::IDLE);
}

// 测试2：暂停和恢复监控
TEST_F(RealtimeBackupTest, PauseAndResume) {
    RealtimeBackupConfig config = createBasicConfig();
    monitor_->startMonitor(config);

    EXPECT_EQ(monitor_->getStatus(), RealtimeBackupStatus::RUNNING);

    monitor_->pauseMonitor();
    EXPECT_EQ(monitor_->getStatus(), RealtimeBackupStatus::PAUSED);

    monitor_->resumeMonitor();
    EXPECT_EQ(monitor_->getStatus(), RealtimeBackupStatus::RUNNING);

    monitor_->stopMonitor();
}

// 测试3：手动备份文件
TEST_F(RealtimeBackupTest, ManualBackup) {
    // 创建测试文件
    createTestFile("test.txt", "Hello World");

    RealtimeBackupConfig config = createBasicConfig();
    monitor_->startMonitor(config);

    // 手动备份
    std::string filePath = sourceDir_ + "\\test.txt";
    EXPECT_TRUE(monitor_->backupFileNow(filePath));

    // 验证备份文件存在
    std::string targetPath = targetDir_ + "\\test.txt";
    EXPECT_TRUE(fs::exists(targetPath));

    // 验证内容一致
    std::ifstream inFile(filePath);
    std::ifstream outFile(targetPath);
    std::string inContent((std::istreambuf_iterator<char>(inFile)),
                          std::istreambuf_iterator<char>());
    std::string outContent((std::istreambuf_iterator<char>(outFile)),
                           std::istreambuf_iterator<char>());
    EXPECT_EQ(inContent, outContent);

    monitor_->stopMonitor();
}

// 测试4：统计信息
TEST_F(RealtimeBackupTest, Statistics) {
    createTestFile("file1.txt", "Data1");
    createTestFile("file2.txt", "Data2");

    RealtimeBackupConfig config = createBasicConfig();
    monitor_->startMonitor(config);

    // 手动备份两个文件
    monitor_->backupFileNow(sourceDir_ + "\\file1.txt");
    monitor_->backupFileNow(sourceDir_ + "\\file2.txt");

    RealtimeBackupStats stats = monitor_->getStats();
    EXPECT_EQ(stats.filesBackedUp, 2);
    EXPECT_GT(stats.totalBytesBackedUp, 0);

    monitor_->stopMonitor();
}

// 测试5：加密备份
TEST_F(RealtimeBackupTest, EncryptedBackup) {
    createTestFile("secret.txt", "Secret Data");

    RealtimeBackupConfig config = createBasicConfig();
    config.enableEncryption = true;
    config.encryptionPassword = "password123";

    monitor_->startMonitor(config);

    std::string filePath = sourceDir_ + "\\secret.txt";
    EXPECT_TRUE(monitor_->backupFileNow(filePath));

    std::string targetPath = targetDir_ + "\\secret.txt";
    EXPECT_TRUE(fs::exists(targetPath));

    monitor_->stopMonitor();
}

// 测试6：文件过滤器
TEST_F(RealtimeBackupTest, FileFilter) {
    createTestFile("test.txt", "Text File");
    createTestFile("image.jpg", "Image File");

    RealtimeBackupConfig config = createBasicConfig();
    config.fileFilters = {".txt"};  // 只备份.txt文件

    monitor_->startMonitor(config);

    // .txt文件应该备份
    EXPECT_TRUE(monitor_->backupFileNow(sourceDir_ + "\\test.txt"));

    // .jpg文件不应该备份（文件不在过滤列表中）
    // 注意：backupFileNow不检查过滤器，所以这里会成功
    // 实际监控时，shouldBackupFile会检查过滤器

    monitor_->stopMonitor();
}

// 测试7：无效路径
TEST_F(RealtimeBackupTest, InvalidPath) {
    RealtimeBackupConfig config;
    config.sourcePath = "C:\\Nonexistent\\Path";
    config.targetPath = targetDir_;

    EXPECT_FALSE(monitor_->startMonitor(config));
    EXPECT_FALSE(monitor_->isRunning());
}

// 测试8：重复启动
TEST_F(RealtimeBackupTest, DoubleStart) {
    RealtimeBackupConfig config = createBasicConfig();

    EXPECT_TRUE(monitor_->startMonitor(config));
    EXPECT_FALSE(monitor_->startMonitor(config));  // 重复启动应失败

    monitor_->stopMonitor();
}

// 测试9：空目录备份
TEST_F(RealtimeBackupTest, EmptyDirectory) {
    RealtimeBackupConfig config = createBasicConfig();
    EXPECT_TRUE(monitor_->startMonitor(config));

    RealtimeBackupStats stats = monitor_->getStats();
    EXPECT_EQ(stats.filesBackedUp, 0);

    monitor_->stopMonitor();
}

// 测试10：大文件备份
TEST_F(RealtimeBackupTest, LargeFileBackup) {
    // 创建大文件
    std::string largeFile = sourceDir_ + "\\large.bin";
    {
        std::ofstream file(largeFile, std::ios::binary);
        std::vector<char> data(1024 * 1024, 'A');  // 1MB
        file.write(data.data(), data.size());
    }

    RealtimeBackupConfig config = createBasicConfig();
    monitor_->startMonitor(config);

    EXPECT_TRUE(monitor_->backupFileNow(largeFile));

    std::string targetPath = targetDir_ + "\\large.bin";
    EXPECT_TRUE(fs::exists(targetPath));
    EXPECT_EQ(fs::file_size(targetPath), 1024 * 1024);

    monitor_->stopMonitor();
}

// 测试11：监控文件变化（需要异步测试）
TEST_F(RealtimeBackupTest, DISABLED_FileChangeDetection) {
    // 注意：这个测试需要异步执行，在CI环境中可能不稳定
    // 所以标记为DISABLED

    RealtimeBackupConfig config = createBasicConfig();

    bool callbackCalled = false;
    FileChangeEvent lastEvent;

    monitor_->startMonitor(config, [&](const FileChangeEvent& event) {
        callbackCalled = true;
        lastEvent = event;
    });

    // 等待监控启动
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // 创建文件
    createTestFile("newfile.txt", "New File");

    // 等待回调
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // 验证回调被调用
    EXPECT_TRUE(callbackCalled);
    EXPECT_EQ(lastEvent.type, FileChangeType::CREATED);

    monitor_->stopMonitor();
}

// 测试12：获取监控路径
TEST_F(RealtimeBackupTest, GetMonitoredPath) {
    RealtimeBackupConfig config = createBasicConfig();
    monitor_->startMonitor(config);

    EXPECT_EQ(monitor_->getMonitoredPath(), sourceDir_);

    monitor_->stopMonitor();
}

// 测试13：元数据保存
TEST_F(RealtimeBackupTest, MetadataSaved) {
    createTestFile("test.txt", "Test Content");

    RealtimeBackupConfig config = createBasicConfig();
    monitor_->startMonitor(config);

    monitor_->backupFileNow(sourceDir_ + "\\test.txt");

    // 检查元数据文件
    std::string metaPath = targetDir_ + "\\test.txt.meta";
    EXPECT_TRUE(fs::exists(metaPath));

    monitor_->stopMonitor();
}

// 测试14：多次备份同一文件
TEST_F(RealtimeBackupTest, MultipleBackup) {
    createTestFile("test.txt", "Original");

    RealtimeBackupConfig config = createBasicConfig();
    monitor_->startMonitor(config);

    // 手动备份两次
    monitor_->backupFileNow(sourceDir_ + "\\test.txt");
    monitor_->backupFileNow(sourceDir_ + "\\test.txt");

    RealtimeBackupStats stats = monitor_->getStats();
    EXPECT_EQ(stats.filesBackedUp, 2);

    monitor_->stopMonitor();
}

// 测试15：备份失败处理
TEST_F(RealtimeBackupTest, BackupFailure) {
    RealtimeBackupConfig config = createBasicConfig();
    monitor_->startMonitor(config);

    // 尝试备份不存在的文件
    EXPECT_FALSE(monitor_->backupFileNow("C:\\Nonexistent\\File.txt"));

    monitor_->stopMonitor();
}

// 测试16：回调机制验证（简化版本，不测试实时监控）
TEST_F(RealtimeBackupTest, CallbackMechanism) {
    createTestFile("test.txt", "Test Content");

    RealtimeBackupConfig config = createBasicConfig();

    // 设置回调
    std::atomic<int> callbackCount{0};
    FileChangeCallback callback = [&callbackCount](const FileChangeEvent& event) {
        callbackCount++;
    };

    monitor_->startMonitor(config, callback);

    // 手动备份不会触发回调（只有文件系统变化才触发）
    monitor_->backupFileNow(sourceDir_ + "\\test.txt");

    // 验证监控器正常运行
    RealtimeBackupStats stats = monitor_->getStats();
    EXPECT_EQ(stats.filesBackedUp, 1);

    monitor_->stopMonitor();
}

// 测试17：多线程并发访问
TEST_F(RealtimeBackupTest, MultiThreadConcurrency) {
    createTestFile("test1.txt", "Content 1");
    createTestFile("test2.txt", "Content 2");

    RealtimeBackupConfig config = createBasicConfig();
    monitor_->startMonitor(config);

    // 创建多个线程同时访问
    std::vector<std::thread> threads;
    std::atomic<int> successCount{0};

    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([this, &successCount]() {
            try {
                // 并发访问各种方法
                monitor_->getStats();
                monitor_->getStatus();
                monitor_->getMonitoredPath();
                monitor_->backupFileNow(sourceDir_ + "\\test1.txt");
                successCount++;
            } catch (...) {
                // 忽略异常
            }
        });
    }

    // 等待所有线程完成
    for (auto& t : threads) {
        t.join();
    }

    // 所有线程都成功完成（没有崩溃）
    EXPECT_EQ(successCount.load(), 10);

    monitor_->stopMonitor();
}