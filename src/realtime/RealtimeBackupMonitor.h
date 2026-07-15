#pragma once
#include "IRealtimeBackupMonitor.h"
#include "encryption/IEncryptionStrategy.h"
#include "core/MetadataManager.h"
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <windows.h>

namespace DataBackup {

/**
 * 实时备份监控器实现类
 *
 * 使用Windows ReadDirectoryChangesW API监控文件变化
 * 线程安全，支持加密，不打包
 */
class RealtimeBackupMonitor : public IRealtimeBackupMonitor {
public:
    /**
     * 构造函数
     */
    RealtimeBackupMonitor();

    /**
     * 析构函数
     */
    ~RealtimeBackupMonitor() override;

    // IRealtimeBackupMonitor接口实现
    bool startMonitor(const RealtimeBackupConfig& config,
                      FileChangeCallback callback = nullptr) override;

    void stopMonitor() override;
    void pauseMonitor() override;
    void resumeMonitor() override;

    RealtimeBackupStatus getStatus() const override;
    RealtimeBackupStats getStats() const override;
    std::string getMonitoredPath() const override;
    bool isRunning() const override;

    bool backupFileNow(const std::string& filePath) override;

private:
    /**
     * 监控线程主函数
     */
    void monitorThread();

    /**
     * 处理文件变化事件
     * @param filePath 源侧文件绝对路径
     * @param action  Windows 通知动作(FILE_ACTION_*)
     */
    void handleFileChange(const std::string& filePath, DWORD action);

    /**
     * 备份单个文件（源 → 目标）
     */
    bool backupFile(const std::string& filePath);

    /**
     * 从目标端删除对应文件（配合 mirrorDelete）
     */
    bool deleteMirrorFile(const std::string& sourceFilePath);

    /**
     * 启动时全量同步：把源目录已有的所有文件复制到目标
     */
    void performInitialSync();

    /**
     * 判断文件是否为目标目录内的产物（防止目标位于源内导致无限循环）
     */
    bool isInsideTarget(const std::string& absSource) const;

    /**
     * 检查文件是否符合过滤条件
     */
    bool shouldBackupFile(const std::string& filePath);

    /**
     * 计算源文件在目标端的对应路径
     */
    std::string computeTargetPath(const std::string& sourceFilePath) const;

    /**
     * 获取当前时间戳（毫秒）
     */
    static uint64_t getCurrentTimestamp();

private:
    // 配置和状态（需要锁保护）
    RealtimeBackupConfig config_;
    mutable std::mutex configMutex_;

    RealtimeBackupStatus status_;
    mutable std::mutex statusMutex_;

    RealtimeBackupStats stats_;
    mutable std::mutex statsMutex_;

    // 回调函数（需要锁保护）
    FileChangeCallback callback_;
    mutable std::mutex callbackMutex_;

    // 线程控制
    std::thread monitorThread_;
    std::atomic<bool> running_;
    std::atomic<bool> paused_;
    std::atomic<bool> stopRequested_;

    // Windows句柄
    HANDLE directoryHandle_;
    HANDLE stopEvent_;

    // 加密器（可选）
    std::unique_ptr<IEncryptionStrategy> encryptor_;

    // 元数据管理器
    std::unique_ptr<MetadataManager> metadataManager_;

    // 重命名事件配对：OLD_NAME 先到，NEW_NAME 后到；由监控线程独占，无需加锁
    std::string pendingRenameOldPath_;
};

} // namespace DataBackup