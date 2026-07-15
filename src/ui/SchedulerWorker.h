#pragma once

#include <QObject>
#include <memory>
#include <vector>
#include "scheduler/BackupScheduler.h"
#include "scheduler/ScheduleTypes.h"

namespace DataBackup {

/**
 * 调度器工作类
 * 封装 BackupScheduler，管理定时任务
 */
class SchedulerWorker : public QObject {
    Q_OBJECT

public:
    /**
     * 构造函数
     * @param parent 父对象
     */
    explicit SchedulerWorker(QObject* parent = nullptr);

    /**
     * 析构函数
     */
    ~SchedulerWorker() override;

    /**
     * 启动调度器
     */
    void start();

    /**
     * 停止调度器
     */
    void stop();

    /**
     * 添加任务
     * @param config 任务配置
     * @return 是否成功
     */
    bool addTask(const ScheduleConfig& config);

    /**
     * 删除任务
     * @param taskId 任务ID
     * @return 是否成功
     */
    bool removeTask(const std::string& taskId);

    /**
     * 暂停任务
     * @param taskId 任务ID
     * @return 是否成功
     */
    bool pauseTask(const std::string& taskId);

    /**
     * 恢复任务
     * @param taskId 任务ID
     * @return 是否成功
     */
    bool resumeTask(const std::string& taskId);

    /**
     * 执行任务
     * @param taskId 任务ID
     */
    void executeTask(const std::string& taskId);

    /**
     * 获取任务列表
     * @return 任务列表
     */
    std::vector<TaskInfo> getTaskList() const;

    /**
     * 检查是否正在运行
     * @return 是否正在运行
     */
    bool isRunning() const;

signals:
    /**
     * 任务执行完成信号
     * @param result 执行结果
     */
    void taskExecuted(const TaskExecutionResult& result);

private:
    std::unique_ptr<BackupScheduler> scheduler_; // 调度器
};

} // namespace DataBackup