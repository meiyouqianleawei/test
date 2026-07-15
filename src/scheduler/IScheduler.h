#pragma once
#include "ScheduleTypes.h"
#include <vector>
#include <memory>

namespace DataBackup {

/**
 * 定时调度器接口
 */
class IScheduler {
public:
    virtual ~IScheduler() = default;

    /**
     * 添加定时任务
     * @param config 任务配置
     * @return 是否成功添加
     */
    virtual bool addTask(const ScheduleConfig& config) = 0;

    /**
     * 删除定时任务
     * @param taskId 任务ID
     * @return 是否成功删除
     */
    virtual bool removeTask(const std::string& taskId) = 0;

    /**
     * 暂停定时任务
     * @param taskId 任务ID
     * @return 是否成功暂停
     */
    virtual bool pauseTask(const std::string& taskId) = 0;

    /**
     * 恢复定时任务
     * @param taskId 任务ID
     * @return 是否成功恢复
     */
    virtual bool resumeTask(const std::string& taskId) = 0;

    /**
     * 启动调度器
     */
    virtual void start() = 0;

    /**
     * 停止调度器
     */
    virtual void stop() = 0;

    /**
     * 获取所有任务列表
     * @return 任务信息列表
     */
    virtual std::vector<TaskInfo> getTaskList() const = 0;

    /**
     * 获取指定任务信息
     * @param taskId 任务ID
     * @return 任务信息
     */
    virtual TaskInfo getTaskInfo(const std::string& taskId) const = 0;

    /**
     * 设置任务回调
     * @param taskId 任务ID
     * @param callback 回调函数
     */
    virtual void setTaskCallback(const std::string& taskId, TaskCallback callback) = 0;

    /**
     * 检查调度器是否正在运行
     * @return 是否正在运行
     */
    virtual bool isRunning() const = 0;

    /**
     * 保存任务配置到文件
     * @param filePath 配置文件路径
     * @return 是否成功保存
     */
    virtual bool saveTasks(const std::string& filePath) = 0;

    /**
     * 从文件加载任务配置
     * @param filePath 配置文件路径
     * @return 是否成功加载
     */
    virtual bool loadTasks(const std::string& filePath) = 0;

    /**
     * 手动触发任务执行（用于测试）
     * @param taskId 任务ID
     * @return 执行结果
     */
    virtual TaskExecutionResult executeTask(const std::string& taskId) = 0;
};

} // namespace DataBackup