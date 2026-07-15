#pragma once
#include "RealtimeBackupTypes.h"
#include <functional>
#include <memory>
#include <vector>

namespace DataBackup {

/**
 * 实时备份监控回调函数类型
 */
using FileChangeCallback = std::function<void(const FileChangeEvent&)>;

/**
 * 实时备份监控器接口
 *
 * 实时监控文件变化并自动备份（不打包）
 */
class IRealtimeBackupMonitor {
public:
    virtual ~IRealtimeBackupMonitor() = default;

    /**
     * 启动监控（线程安全）
     * @param config 监控配置
     * @param callback 文件变化回调（可选）
     * @return 是否成功启动
     */
    virtual bool startMonitor(const RealtimeBackupConfig& config,
                              FileChangeCallback callback = nullptr) = 0;

    /**
     * 停止监控（线程安全）
     */
    virtual void stopMonitor() = 0;

    /**
     * 暂停监控（线程安全）
     */
    virtual void pauseMonitor() = 0;

    /**
     * 恢复监控（线程安全）
     */
    virtual void resumeMonitor() = 0;

    /**
     * 获取监控状态（线程安全）
     * @return 当前状态
     */
    virtual RealtimeBackupStatus getStatus() const = 0;

    /**
     * 获取统计信息（线程安全）
     * @return 统计信息
     */
    virtual RealtimeBackupStats getStats() const = 0;

    /**
     * 获取监控的目录路径（线程安全）
     * @return 监控的目录路径
     */
    virtual std::string getMonitoredPath() const = 0;

    /**
     * 检查是否正在运行（线程安全）
     * @return 是否正在运行
     */
    virtual bool isRunning() const = 0;

    /**
     * 手动触发备份（线程安全）
     * 用于测试或手动备份特定文件
     * @param filePath 要备份的文件路径
     * @return 是否成功备份
     */
    virtual bool backupFileNow(const std::string& filePath) = 0;
};

} // namespace DataBackup