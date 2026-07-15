#include <gtest/gtest.h>
#include "scheduler/BackupScheduler.h"
#include "scheduler/ScheduleTypes.h"
#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>

using namespace DataBackup;
namespace fs = std::filesystem;

class BackupSchedulerTest : public ::testing::Test {
protected:
    std::string testDir_;
    std::string configFile_;
    std::unique_ptr<BackupScheduler> scheduler_;

    void SetUp() override {
        testDir_ = (fs::temp_directory_path() / "BackupSchedulerTest").string();
        configFile_ = (fs::temp_directory_path() / "tasks.ini").string();

        // 清理旧数据
        fs::remove_all(testDir_);
        fs::remove(configFile_);

        // 创建测试目录
        fs::create_directories(testDir_);
        std::ofstream(testDir_ + "/test.txt") << "Test Data";

        // 创建调度器
        scheduler_ = std::make_unique<BackupScheduler>();
    }

    void TearDown() override {
        // 停止调度器
        if (scheduler_ && scheduler_->isRunning()) {
            scheduler_->stop();
        }

        // 清理测试数据
        fs::remove_all(testDir_);
        fs::remove(configFile_);
    }

    // 辅助方法：创建基本任务配置
    ScheduleConfig createBasicConfig() {
        ScheduleConfig config;
        config.taskId = "TEST_001";
        config.taskName = "Test Task";
        config.type = ScheduleType::ONCE;
        config.hour = 23;
        config.minute = 59;
        config.enabled = true;
        config.backupConfig.sourcePath = testDir_;
        config.backupConfig.targetPath = testDir_ + "/backup.pkg";
        return config;
    }
};

// 测试1：创建和启动调度器
TEST_F(BackupSchedulerTest, CreateAndStart) {
    EXPECT_FALSE(scheduler_->isRunning());

    scheduler_->start();
    EXPECT_TRUE(scheduler_->isRunning());

    scheduler_->stop();
    EXPECT_FALSE(scheduler_->isRunning());
}

// 测试2：添加和删除任务
TEST_F(BackupSchedulerTest, AddAndRemoveTask) {
    ScheduleConfig config = createBasicConfig();

    EXPECT_TRUE(scheduler_->addTask(config));
    EXPECT_FALSE(scheduler_->addTask(config)); // 重复添加应失败

    auto tasks = scheduler_->getTaskList();
    EXPECT_EQ(tasks.size(), 1);
    EXPECT_EQ(tasks[0].config.taskId, config.taskId);

    EXPECT_TRUE(scheduler_->removeTask(config.taskId));
    EXPECT_FALSE(scheduler_->removeTask(config.taskId)); // 删除不存在的任务应失败

    tasks = scheduler_->getTaskList();
    EXPECT_EQ(tasks.size(), 0);
}

// 测试3：暂停和恢复任务
TEST_F(BackupSchedulerTest, PauseAndResumeTask) {
    ScheduleConfig config = createBasicConfig();
    scheduler_->addTask(config);

    EXPECT_TRUE(scheduler_->pauseTask(config.taskId));

    TaskInfo info = scheduler_->getTaskInfo(config.taskId);
    EXPECT_EQ(info.status, TaskStatus::PAUSED);

    EXPECT_TRUE(scheduler_->resumeTask(config.taskId));

    info = scheduler_->getTaskInfo(config.taskId);
    EXPECT_EQ(info.status, TaskStatus::IDLE);
}

// 测试4：手动执行任务
TEST_F(BackupSchedulerTest, ExecuteTaskManually) {
    ScheduleConfig config = createBasicConfig();
    scheduler_->addTask(config);

    TaskExecutionResult result = scheduler_->executeTask(config.taskId);

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.taskId, config.taskId);
    EXPECT_GT(result.duration, 0);
}

// 测试5：任务回调
TEST_F(BackupSchedulerTest, TaskCallback) {
    ScheduleConfig config = createBasicConfig();
    scheduler_->addTask(config);

    bool callbackCalled = false;
    TaskExecutionResult callbackResult;

    scheduler_->setTaskCallback(config.taskId, [&](const TaskExecutionResult& result) {
        callbackCalled = true;
        callbackResult = result;
    });

    TaskExecutionResult result = scheduler_->executeTask(config.taskId);

    EXPECT_TRUE(callbackCalled);
    EXPECT_EQ(callbackResult.taskId, config.taskId);
    EXPECT_EQ(callbackResult.success, result.success);
}

// 测试6：保存和加载任务配置
TEST_F(BackupSchedulerTest, SaveAndLoadTasks) {
    ScheduleConfig config1 = createBasicConfig();
    ScheduleConfig config2 = createBasicConfig();
    config2.taskId = "TEST_002";
    config2.taskName = "Test Task 2";

    scheduler_->addTask(config1);
    scheduler_->addTask(config2);

    EXPECT_TRUE(scheduler_->saveTasks(configFile_));
    EXPECT_TRUE(fs::exists(configFile_));

    // 创建新调度器并加载
    auto newScheduler = std::make_unique<BackupScheduler>();
    EXPECT_TRUE(newScheduler->loadTasks(configFile_));

    auto tasks = newScheduler->getTaskList();
    EXPECT_EQ(tasks.size(), 2);
}

// 测试7：获取任务列表
TEST_F(BackupSchedulerTest, GetTaskList) {
    ScheduleConfig config1 = createBasicConfig();
    ScheduleConfig config2 = createBasicConfig();
    config2.taskId = "TEST_002";

    scheduler_->addTask(config1);
    scheduler_->addTask(config2);

    auto tasks = scheduler_->getTaskList();
    EXPECT_EQ(tasks.size(), 2);

    // 检查任务是否存在
    bool found1 = false, found2 = false;
    for (const auto& task : tasks) {
        if (task.config.taskId == config1.taskId) found1 = true;
        if (task.config.taskId == config2.taskId) found2 = true;
    }
    EXPECT_TRUE(found1);
    EXPECT_TRUE(found2);
}

// 测试8：自动生成任务ID
TEST_F(BackupSchedulerTest, AutoGenerateTaskId) {
    ScheduleConfig config = createBasicConfig();
    config.taskId = ""; // 空ID，应自动生成

    EXPECT_TRUE(scheduler_->addTask(config));

    auto tasks = scheduler_->getTaskList();
    EXPECT_EQ(tasks.size(), 1);
    EXPECT_FALSE(tasks[0].config.taskId.empty());
}

// 测试9：不存在的任务操作
TEST_F(BackupSchedulerTest, NonexistentTaskOperations) {
    EXPECT_FALSE(scheduler_->removeTask("NONEXISTENT"));
    EXPECT_FALSE(scheduler_->pauseTask("NONEXISTENT"));
    EXPECT_FALSE(scheduler_->resumeTask("NONEXISTENT"));

    TaskInfo info = scheduler_->getTaskInfo("NONEXISTENT");
    EXPECT_TRUE(info.config.taskId.empty());
}

// 测试10：执行历史记录
TEST_F(BackupSchedulerTest, ExecutionHistory) {
    ScheduleConfig config = createBasicConfig();
    scheduler_->addTask(config);

    // 执行多次
    for (int i = 0; i < 3; ++i) {
        scheduler_->executeTask(config.taskId);
    }

    TaskInfo info = scheduler_->getTaskInfo(config.taskId);
    EXPECT_GE(info.history.size(), 3);
}

// 测试11：启用/禁用任务
TEST_F(BackupSchedulerTest, EnableDisableTask) {
    ScheduleConfig config = createBasicConfig();
    config.enabled = false;

    scheduler_->addTask(config);

    TaskInfo info = scheduler_->getTaskInfo(config.taskId);
    EXPECT_FALSE(info.config.enabled);

    // 修改为启用
    config.enabled = true;
    scheduler_->removeTask(config.taskId);
    scheduler_->addTask(config);

    info = scheduler_->getTaskInfo(config.taskId);
    EXPECT_TRUE(info.config.enabled);
}

// 测试12：不同任务类型
TEST_F(BackupSchedulerTest, DifferentScheduleTypes) {
    ScheduleConfig onceConfig = createBasicConfig();
    onceConfig.type = ScheduleType::ONCE;

    ScheduleConfig dailyConfig = createBasicConfig();
    dailyConfig.taskId = "DAILY_001";
    dailyConfig.type = ScheduleType::DAILY;

    ScheduleConfig weeklyConfig = createBasicConfig();
    weeklyConfig.taskId = "WEEKLY_001";
    weeklyConfig.type = ScheduleType::WEEKLY;
    weeklyConfig.dayOfWeek = DayOfWeek::FRIDAY;

    EXPECT_TRUE(scheduler_->addTask(onceConfig));
    EXPECT_TRUE(scheduler_->addTask(dailyConfig));
    EXPECT_TRUE(scheduler_->addTask(weeklyConfig));

    auto tasks = scheduler_->getTaskList();
    EXPECT_EQ(tasks.size(), 3);
}

// 测试13：间隔任务
TEST_F(BackupSchedulerTest, IntervalTask) {
    ScheduleConfig config = createBasicConfig();
    config.type = ScheduleType::INTERVAL;
    config.intervalSeconds = 5; // 5秒间隔

    EXPECT_TRUE(scheduler_->addTask(config));

    TaskInfo info = scheduler_->getTaskInfo(config.taskId);
    EXPECT_GT(info.nextExecutionTime, 0);
}

// 测试14：任务执行失败处理
TEST_F(BackupSchedulerTest, TaskExecutionFailure) {
    ScheduleConfig config = createBasicConfig();
    config.backupConfig.sourcePath = "/nonexistent/path"; // 不存在的源路径

    scheduler_->addTask(config);

    TaskExecutionResult result = scheduler_->executeTask(config.taskId);

    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.errorMessage.empty());
}

// 测试15：大文件备份任务
TEST_F(BackupSchedulerTest, LargeFileBackup) {
    // 创建大文件
    std::string largeFile = testDir_ + "/large.bin";
    {
        std::ofstream file(largeFile, std::ios::binary);
        std::vector<char> data(1024 * 1024, 'A'); // 1MB
        file.write(data.data(), data.size());
    }

    ScheduleConfig config = createBasicConfig();
    scheduler_->addTask(config);

    TaskExecutionResult result = scheduler_->executeTask(config.taskId);

    EXPECT_TRUE(result.success);
    EXPECT_GT(result.backupSize, 1024 * 1024); // 至少1MB
}

// 测试16：回调中调用调度器方法（验证不死锁）
TEST_F(BackupSchedulerTest, CallbackNoDeadlock) {
    ScheduleConfig config = createBasicConfig();
    scheduler_->addTask(config);

    bool callbackCalled = false;
    bool deadlockDetected = false;

    scheduler_->setTaskCallback(config.taskId, [&](const TaskExecutionResult& result) {
        callbackCalled = true;

        // 在回调中尝试调用调度器方法（如果死锁会卡住）
        try {
            auto tasks = scheduler_->getTaskList();  // 尝试获取任务列表
            TaskInfo info = scheduler_->getTaskInfo(config.taskId);  // 尝试获取任务信息

            // 如果能成功调用，说明没有死锁
            deadlockDetected = false;
        } catch (...) {
            deadlockDetected = true;
        }
    });

    // 执行任务
    auto start = std::chrono::high_resolution_clock::now();
    TaskExecutionResult result = scheduler_->executeTask(config.taskId);
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    // 验证回调被调用
    EXPECT_TRUE(callbackCalled);

    // 验证没有死锁（如果死锁会超时超过5秒）
    EXPECT_LT(duration.count(), 5000);

    // 验证回调中的调用成功
    EXPECT_FALSE(deadlockDetected);
}

// 测试17：多线程并发调用（验证线程安全）
TEST_F(BackupSchedulerTest, MultiThreadConcurrency) {
    // 创建多个任务
    std::vector<std::string> taskIds;
    for (int i = 0; i < 5; ++i) {
        ScheduleConfig config = createBasicConfig();
        config.taskId = "TASK_" + std::to_string(i);
        config.taskName = "Task " + std::to_string(i);
        scheduler_->addTask(config);
        taskIds.push_back(config.taskId);
    }

    // 启动多个线程并发调用调度器方法
    std::vector<std::thread> threads;
    std::atomic<int> successCount(0);

    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&, i]() {
            try {
                // 随机调用不同方法
                if (i % 3 == 0) {
                    auto tasks = scheduler_->getTaskList();
                    successCount++;
                } else if (i % 3 == 1) {
                    if (!taskIds.empty()) {
                        TaskInfo info = scheduler_->getTaskInfo(taskIds[0]);
                        successCount++;
                    }
                } else {
                    if (!taskIds.empty()) {
                        scheduler_->pauseTask(taskIds[0]);
                        scheduler_->resumeTask(taskIds[0]);
                        successCount++;
                    }
                }
            } catch (...) {
                // 异常说明线程安全问题
            }
        });
    }

    // 等待所有线程完成
    for (auto& thread : threads) {
        thread.join();
    }

    // 验证所有线程都成功
    EXPECT_EQ(successCount.load(), 10);
}