#pragma once
#include "BackupTypes.h"
#include "pack/Packager.h"
#include <functional>
#include <memory>
#include <atomic>

namespace DataBackup {

/**
 * 还原引擎类
 * 实现备份文件的还原流程
 */
class RestoreEngine {
public:
    /**
     * 进度回调函数类型
     */
    using ProgressCallback = std::function<void(const BackupProgress&)>;

    /**
     * 构造函数
     */
    RestoreEngine();

    /**
     * 析构函数
     */
    ~RestoreEngine();

    /**
     * 还原备份
     * @param backupFilePath 备份文件路径
     * @param targetDirectory 目标还原目录
     * @param password 解密密码（如果加密了）
     * @param progressCallback 进度回调（可选）
     * @return 还原结果
     */
    RestoreResult restore(const std::string& backupFilePath,
                          const std::string& targetDirectory,
                          const std::string& password = "",
                          ProgressCallback progressCallback = nullptr);

    /**
     * 验证备份文件
     * @param backupFilePath 备份文件路径
     * @return 是否有效
     */
    bool verifyBackup(const std::string& backupFilePath);

    /**
     * 获取备份文件信息
     * @param backupFilePath 备份文件路径
     * @return 备份信息（JSON格式）
     */
    std::string getBackupInfo(const std::string& backupFilePath);

    /**
     * 取消还原
     * @return 是否成功取消
     */
    bool cancelRestore();

    /**
     * 获取当前进度
     * @return 当前进度信息
     */
    BackupProgress getProgress() const;

    /**
     * 设置进度回调
     * @param callback 进度回调函数
     */
    void setProgressCallback(ProgressCallback callback);

    /**
     * 检查是否正在还原
     * @return 是否正在还原
     */
    bool isRunning() const;

private:
    // 内部实现方法
    bool decryptAndDecompress(const std::string& password);
    bool restoreMetadata();

    void updateProgress(BackupPhase phase, uint32_t filesProcessed, uint32_t totalFiles);
    void reportError(const std::string& message);

private:
    // 成员变量
    BackupProgress progress_;
    ProgressCallback progressCallback_;

    std::atomic<bool> cancelled_;
    std::atomic<bool> running_;

    std::unique_ptr<Packager> packager_;
};

} // namespace DataBackup