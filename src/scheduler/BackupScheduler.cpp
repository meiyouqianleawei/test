#include "BackupScheduler.h"
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <random>
#include <fstream>

namespace DataBackup {

// 辅助函数：获取当前时间戳（毫秒）
static uint64_t getCurrentTimestamp() {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );
}

// 辅助函数：时间戳转换为可读字符串
static std::string timestampToString(uint64_t timestamp) {
    auto time = std::chrono::system_clock::from_time_t(timestamp / 1000);
    std::time_t timeT = std::chrono::system_clock::to_time_t(time);
    std::tm tmStruct;
    localtime_s(&tmStruct, &timeT);  // 使用安全的localtime_s
    std::stringstream ss;
    ss << std::put_time(&tmStruct, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

BackupScheduler::BackupScheduler()
    : running_(false)
    , stopRequested_(false)
    , backupEngine_(std::make_unique<BackupEngine>()) {
}

BackupScheduler::~BackupScheduler() {
    stop();
}

bool BackupScheduler::addTask(const ScheduleConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);

    // 创建任务信息
    TaskInfo taskInfo;
    taskInfo.config = config;
    taskInfo.status = TaskStatus::IDLE;
    taskInfo.retryCount = 0;

    // 如果没有指定任务ID，生成一个（保证不与现有ID冲突）
    if (taskInfo.config.taskId.empty()) {
        do {
            taskInfo.config.taskId = generateTaskId();
        } while (tasks_.find(taskInfo.config.taskId) != tasks_.end());
    } else {
        // 检查任务ID是否已存在
        if (tasks_.find(taskInfo.config.taskId) != tasks_.end()) {
            return false;
        }
    }

    // 计算下次执行时间（使用带生成ID的最终配置）
    taskInfo.nextExecutionTime = calculateNextExecutionTime(taskInfo.config);

    // 使用最终的 taskId 作为 map 键
    tasks_[taskInfo.config.taskId] = taskInfo;
    return true;
}

bool BackupScheduler::removeTask(const std::string& taskId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = tasks_.find(taskId);
    if (it == tasks_.end()) {
        return false;
    }

    tasks_.erase(it);
    callbacks_.erase(taskId);
    return true;
}

bool BackupScheduler::pauseTask(const std::string& taskId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = tasks_.find(taskId);
    if (it == tasks_.end()) {
        return false;
    }

    it->second.status = TaskStatus::PAUSED;
    return true;
}

bool BackupScheduler::resumeTask(const std::string& taskId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = tasks_.find(taskId);
    if (it == tasks_.end()) {
        return false;
    }

    it->second.status = TaskStatus::IDLE;
    // 重新计算下次执行时间
    it->second.nextExecutionTime = calculateNextExecutionTime(it->second.config);
    return true;
}

void BackupScheduler::start() {
    if (running_) {
        return;
    }

    running_ = true;
    stopRequested_ = false;
    schedulerThread_ = std::thread(&BackupScheduler::schedulerLoop, this);
}

void BackupScheduler::stop() {
    if (!running_) {
        return;
    }

    stopRequested_ = true;
    cv_.notify_all();

    if (schedulerThread_.joinable()) {
        schedulerThread_.join();
    }

    running_ = false;
}

std::vector<TaskInfo> BackupScheduler::getTaskList() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<TaskInfo> result;
    for (const auto& pair : tasks_) {
        result.push_back(pair.second);
    }
    return result;
}

TaskInfo BackupScheduler::getTaskInfo(const std::string& taskId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = tasks_.find(taskId);
    if (it == tasks_.end()) {
        return TaskInfo();
    }
    return it->second;
}

void BackupScheduler::setTaskCallback(const std::string& taskId, TaskCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    callbacks_[taskId] = callback;
}

bool BackupScheduler::isRunning() const {
    return running_;
}

bool BackupScheduler::saveTasks(const std::string& filePath) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        std::ofstream file(filePath);
        if (!file.is_open()) {
            return false;
        }

        // 简化版本：保存为文本格式
        for (const auto& pair : tasks_) {
            const TaskInfo& info = pair.second;
            file << "[Task]" << "\n";
            file << "Id=" << info.config.taskId << "\n";
            file << "Name=" << info.config.taskName << "\n";
            file << "Type=" << static_cast<int>(info.config.type) << "\n";
            file << "Hour=" << info.config.hour << "\n";
            file << "Minute=" << info.config.minute << "\n";
            file << "Enabled=" << info.config.enabled << "\n";
            file << "Source=" << info.config.backupConfig.sourcePath << "\n";
            file << "Target=" << info.config.backupConfig.targetPath << "\n";
            file << "\n";
        }

        return true;
    } catch (...) {
        return false;
    }
}

bool BackupScheduler::loadTasks(const std::string& filePath) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            return false;
        }

        // 简化版本：从文本格式加载
        tasks_.clear();

        std::string line;
        ScheduleConfig config;
        while (std::getline(file, line)) {
            if (line == "[Task]") {
                config = ScheduleConfig();
            } else if (line.find("Id=") == 0) {
                config.taskId = line.substr(3);
            } else if (line.find("Name=") == 0) {
                config.taskName = line.substr(5);
            } else if (line.find("Type=") == 0) {
                config.type = static_cast<ScheduleType>(std::stoi(line.substr(5)));
            } else if (line.find("Hour=") == 0) {
                config.hour = std::stoi(line.substr(5));
            } else if (line.find("Minute=") == 0) {
                config.minute = std::stoi(line.substr(7));
            } else if (line.find("Enabled=") == 0) {
                config.enabled = (line.substr(8) == "1");
            } else if (line.find("Source=") == 0) {
                config.backupConfig.sourcePath = line.substr(7);
            } else if (line.find("Target=") == 0) {
                config.backupConfig.targetPath = line.substr(7);
                // 添加任务
                TaskInfo info;
                info.config = config;
                info.status = TaskStatus::IDLE;
                info.nextExecutionTime = calculateNextExecutionTime(config);
                tasks_[config.taskId] = info;
            }
        }

        return true;
    } catch (...) {
        return false;
    }
}

TaskExecutionResult BackupScheduler::executeTask(const std::string& taskId) {
    // 先获取任务信息并标记为运行中（加锁）
    ScheduleConfig config;
    TaskCallback callback;
    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = tasks_.find(taskId);
        if (it == tasks_.end()) {
            TaskExecutionResult result;
            result.taskId = taskId;
            result.success = false;
            result.errorMessage = "Task not found";
            return result;
        }

        config = it->second.config;
        it->second.status = TaskStatus::RUNNING;

        // 获取回调（如果有）
        auto callbackIt = callbacks_.find(taskId);
        if (callbackIt != callbacks_.end()) {
            callback = callbackIt->second;
        }
    }
    // 离开作用域后释放锁，避免死锁
    
    // 执行备份（不持有锁）
    BackupResult backupResult = backupEngine_->createBackup(config.backupConfig);
    
    // 更新任务状态和执行历史（加锁）
    TaskExecutionResult result;
    result.taskId = taskId;
    result.taskName = config.taskName;
    result.success = backupResult.success;
    result.errorMessage = backupResult.errorMessage;
    result.duration = backupResult.duration;
    result.filesBackedup = backupResult.filesBackedup;
    result.backupSize = backupResult.totalSize;
    result.executionTime = getCurrentTimestamp();
    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = tasks_.find(taskId);
        if (it != tasks_.end()) {
            TaskInfo& info = it->second;
            
            // 更新状态
            if (result.success) {
                info.status = TaskStatus::COMPLETED;
                info.retryCount = 0;
            } else {
                info.retryCount++;
                if (info.retryCount >= info.config.maxRetries) {
                    info.status = TaskStatus::FAILED;
                }
            }
            
            // 更新执行历史
            info.history.push_back(result);
            if (info.history.size() > 100) {
                info.history.erase(info.history.begin());
            }
            
            info.lastExecutionTime = result.executionTime;
        }
    }
    // 离开作用域后释放锁
    
    // 调用回调（不持有锁，避免死锁）
    if (callback) {
        callback(result);
    }
    
    return result;
}

void BackupScheduler::schedulerLoop() {
    while (!stopRequested_) {
        checkAndExecuteTasks();

        // 等待1秒或直到被通知
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait_for(lock, std::chrono::seconds(1), [this] { return stopRequested_.load(); });
    }
}

void BackupScheduler::checkAndExecuteTasks() {
    uint64_t currentTime = getCurrentTimestamp();

    // 收集需要执行的任务（加锁）
    std::vector<std::string> tasksToExecute;
    {
        std::lock_guard<std::mutex> lock(mutex_);

        for (const auto& pair : tasks_) {
            const TaskInfo& info = pair.second;

            // 检查任务是否需要执行
            // nextExecutionTime == 0 表示已过期/不再执行，跳过
            if (info.config.enabled &&
                info.status != TaskStatus::PAUSED &&
                info.status != TaskStatus::RUNNING &&
                info.nextExecutionTime != 0 &&
                info.nextExecutionTime <= currentTime) {

                tasksToExecute.push_back(info.config.taskId);
            }
        }
    }
    // 离开作用域后释放锁

    // 执行任务（不持有锁）
    for (const auto& taskId : tasksToExecute) {
        executeTask(taskId);
    }

    // 更新下次执行时间（加锁）
    {
        std::lock_guard<std::mutex> lock(mutex_);

        for (const auto& taskId : tasksToExecute) {
            auto it = tasks_.find(taskId);
            if (it != tasks_.end()) {
                // 一次性任务执行后不再触发
                if (it->second.config.type == ScheduleType::ONCE) {
                    it->second.config.enabled = false;
                    it->second.nextExecutionTime = 0;
                } else {
                    it->second.nextExecutionTime = calculateNextExecutionTime(it->second.config);
                }
            }
        }
    }
}

uint64_t BackupScheduler::calculateNextExecutionTime(const ScheduleConfig& config) const {
    // 获取当前时间
    auto now = std::chrono::system_clock::now();
    auto nowTime = std::chrono::system_clock::to_time_t(now);
    std::tm nowTm;
    localtime_s(&nowTm, &nowTime);  // 使用安全的localtime_s

    // 计算下次执行时间
    std::tm nextTm = nowTm;
    nextTm.tm_hour = config.hour;
    nextTm.tm_min = config.minute;
    nextTm.tm_sec = config.second;

    switch (config.type) {
        case ScheduleType::ONCE: {
            // 一次性任务：如果时间已过，返回0表示不再执行
            auto nextTime = std::mktime(&nextTm);
            if (nextTime <= std::mktime(&nowTm)) {
                return 0; // 已过期
            }
            break;
        }

        case ScheduleType::DAILY: {
            // 每天执行：如果今天时间已过，计算明天
            auto nextTime = std::mktime(&nextTm);
            if (nextTime <= std::mktime(&nowTm)) {
                nextTm.tm_mday += 1;
            }
            break;
        }

        case ScheduleType::WEEKLY: {
            // 每周执行
            int targetDay = static_cast<int>(config.dayOfWeek);
            int currentDay = nowTm.tm_wday;
            int daysUntilTarget = targetDay - currentDay;
            if (daysUntilTarget < 0) {
                daysUntilTarget += 7;
            } else if (daysUntilTarget == 0) {
                // 今天，但时间已过
                auto nextTime = std::mktime(&nextTm);
                if (nextTime <= std::mktime(&nowTm)) {
                    daysUntilTarget = 7;
                }
            }
            nextTm.tm_mday += daysUntilTarget;
            break;
        }

        case ScheduleType::MONTHLY: {
            // 每月执行
            nextTm.tm_mday = config.dayOfMonth;
            auto nextTime = std::mktime(&nextTm);
            if (nextTime <= std::mktime(&nowTm)) {
                nextTm.tm_mon += 1;
            }
            break;
        }

        case ScheduleType::INTERVAL: {
            // 间隔执行
            auto duration = std::chrono::hours(config.intervalHours) +
                           std::chrono::minutes(config.intervalMinutes) +
                           std::chrono::seconds(config.intervalSeconds);
            auto next = now + duration;
            return static_cast<uint64_t>(
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    next.time_since_epoch()
                ).count()
            );
        }
    }

    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::from_time_t(std::mktime(&nextTm)).time_since_epoch()
        ).count()
    );
}

std::string BackupScheduler::generateTaskId() const {
    // 使用随机数生成唯一ID
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 999999);

    std::stringstream ss;
    ss << "TASK_" << std::setw(6) << std::setfill('0') << dis(gen);
    return ss.str();
}

// ScheduleTypes.h中的方法实现
uint64_t TaskInfo::calculateNextExecutionTime() const {
    // 简化版本：返回0（需要外部计算）
    return 0;
}

} // namespace DataBackup