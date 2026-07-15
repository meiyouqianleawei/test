#pragma once
#include "IScheduler.h"
#include "engine/BackupEngine.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <map>
#include <atomic>

namespace DataBackup {

/**
 * 备份调度器实现类
 * 使用独立线程管理定时任务
 */
class BackupScheduler : public IScheduler {
public:
    /**
     * 构造函数
     */
    BackupScheduler();

    /**
     * 析构函数
     */
    ~BackupScheduler();

    // IScheduler接口实现
    bool addTask(const ScheduleConfig& config) override;
    bool removeTask(const std::string& taskId) override;
    bool pauseTask(const std::string& taskId) override;
    bool resumeTask(const std::string& taskId) override;
    void start() override;
    void stop() override;
    std::vector<TaskInfo> getTaskList() const override;
    TaskInfo getTaskInfo(const std::string& taskId) const override;
    void setTaskCallback(const std::string& taskId, TaskCallback callback) override;
    bool isRunning() const override;
    bool saveTasks(const std::string& filePath) override;
    bool loadTasks(const std::string& filePath) override;
    TaskExecutionResult executeTask(const std::string& taskId) override;

private:
    /**
     * 调度器主循环
     */
    void schedulerLoop();

    /**
     * 检查并执行到期任务
     */
    void checkAndExecuteTasks();

    /**
     * 更新任务的下次执行时间
     * @param taskId 任务ID
     */
    void updateNextExecutionTime(const std::string& taskId);

    /**
     * 计算下次执行时间
     * @param config 任务配置
     * @return 下次执行时间（时间戳）
     */
    uint64_t calculateNextExecutionTime(const ScheduleConfig& config) const;

    /**
     * 生成唯一任务ID
     * @return 任务ID
     */
    std::string generateTaskId() const;

private:
    std::map<std::string, TaskInfo> tasks_;             // 任务映射
    std::map<std::string, TaskCallback> callbacks_;     // 回调映射
    
    mutable std::mutex mutex_;                          // 互斥锁
    std::condition_variable cv_;                        // 条件变量
    std::thread schedulerThread_;                       // 调度线程
    std::atomic<bool> running_;                         // 运行标志
    std::atomic<bool> stopRequested_;                   // 停止请求标志

    std::unique_ptr<BackupEngine> backupEngine_;        // 备份引擎
};

} // namespace DataBackup