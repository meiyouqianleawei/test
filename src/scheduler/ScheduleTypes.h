#pragma once
#include <string>
#include <cstdint>
#include <chrono>
#include <functional>
#include "engine/BackupConfig.h"

namespace DataBackup {

/**
 * 定时任务类型枚举
 */
enum class ScheduleType {
    ONCE,           // 一次性任务（指定时间执行一次）
    DAILY,          // 每天执行
    WEEKLY,         // 每周执行
    MONTHLY,        // 每月执行
    INTERVAL        // 间隔执行（每隔N小时/分钟）
};

/**
 * 星期枚举（用于周任务）
 */
enum class DayOfWeek {
    SUNDAY = 0,
    MONDAY = 1,
    TUESDAY = 2,
    WEDNESDAY = 3,
    THURSDAY = 4,
    FRIDAY = 5,
    SATURDAY = 6
};

/**
 * 任务状态枚举
 */
enum class TaskStatus {
    IDLE,           // 空闲（未开始）
    RUNNING,        // 运行中
    PAUSED,         // 已暂停
    COMPLETED,      // 已完成
    FAILED          // 失败
};

/**
 * 定时任务配置
 */
struct ScheduleConfig {
    ScheduleType type = ScheduleType::ONCE;     // 任务类型
    std::string taskId;                          // 任务ID（唯一标识）
    std::string taskName;                        // 任务名称
    
    // 时间配置
    int hour = 0;                               // 小时（0-23）
    int minute = 0;                             // 分钟（0-59）
    int second = 0;                             // 秒（0-59）
    
    // 周任务配置
    DayOfWeek dayOfWeek = DayOfWeek::SUNDAY;    // 星期几
    
    // 月任务配置
    int dayOfMonth = 1;                         // 每月的第几天（1-31）
    
    // 间隔任务配置
    int intervalHours = 0;                      // 间隔小时数
    int intervalMinutes = 0;                    // 间隔分钟数
    int intervalSeconds = 0;                    // 间隔秒数
    
    // 执行配置
    int maxRetries = 3;                         // 最大重试次数
    bool enabled = true;                        // 是否启用
    
    // 备份配置
    BackupConfig backupConfig;                  // 备份配置
};

/**
 * 任务执行结果
 */
struct TaskExecutionResult {
    std::string taskId;                         // 任务ID
    std::string taskName;                       // 任务名称
    bool success = false;                       // 是否成功
    std::string errorMessage;                   // 错误信息
    uint64_t executionTime = 0;                 // 执行时间（时间戳）
    uint64_t duration = 0;                      // 耗时（毫秒）
    uint32_t filesBackedup = 0;                 // 备份文件数
    uint64_t backupSize = 0;                    // 备份大小
};

/**
 * 任务信息（包含状态）
 */
struct TaskInfo {
    ScheduleConfig config;                      // 任务配置
    TaskStatus status = TaskStatus::IDLE;      // 任务状态
    uint64_t nextExecutionTime = 0;            // 下次执行时间（时间戳）
    uint64_t lastExecutionTime = 0;            // 上次执行时间（时间戳）
    int retryCount = 0;                        // 当前重试次数
    std::vector<TaskExecutionResult> history;  // 执行历史
    
    /**
     * 计算下次执行时间
     */
    uint64_t calculateNextExecutionTime() const;
};

/**
 * 任务回调函数类型
 */
using TaskCallback = std::function<void(const TaskExecutionResult&)>;

} // namespace DataBackup