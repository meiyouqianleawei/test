#pragma once
#include "BackupConfig.h"
#include "BackupTypes.h"
#include "core/FileScanner.h"
#include "pack/Packager.h"
#include "filter/FilterChain.h"
#include <functional>
#include <memory>
#include <atomic>
#include <mutex>

namespace DataBackup {

/**
 * 备份引擎类（线程安全）
 * 集成所有模块，实现完整的备份流程
 *
 * 线程安全说明：
 * - 所有公共方法都是线程安全的
 * - 进度回调在调用线程执行（建议GUI使用信号槽转发到主线程）
 * - 使用互斥锁保护共享数据（progress_和progressCallback_）
 */
class BackupEngine {
public:
    /**
     * 进度回调函数类型
     * 参数：当前进度
     */
    using ProgressCallback = std::function<void(const BackupProgress&)>;

    /**
     * 构造函数
     */
    BackupEngine();

    /**
     * 析构函数
     */
    ~BackupEngine();

    /**
     * 创建备份
     * @param config 备份配置
     * @param progressCallback 进度回调（可选）
     * @return 备份结果
     */
    BackupResult createBackup(const BackupConfig& config,
                              ProgressCallback progressCallback = nullptr);

    /**
     * 取消备份
     * @return 是否成功取消
     */
    bool cancelBackup();

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
     * 检查是否正在备份
     * @return 是否正在备份
     */
    bool isRunning() const;

private:
    // 内部实现方法
    bool scanFiles(const BackupConfig& config);
    bool filterFiles();
    bool compressAndEncrypt();
    bool packFiles(const BackupConfig& config);
    bool verifyBackup(const BackupConfig& config);

    void updateProgress(BackupPhase phase, uint32_t filesProcessed, uint32_t totalFiles);
    void reportError(const std::string& message);

private:
    // 成员变量
    BackupConfig config_;
    mutable std::mutex progressMutex_;  // 保护progress_和progressCallback_
    BackupProgress progress_;
    ProgressCallback progressCallback_;

    std::vector<FileInfo> scannedFiles_;
    std::vector<FileInfo> filteredFiles_;
    std::vector<FileInfo> filesToBackup_;

    std::atomic<bool> cancelled_;
    std::atomic<bool> running_;

    std::unique_ptr<FileScanner> scanner_;
    std::unique_ptr<Packager> packager_;

    // 备份阶段选择的加密策略(为空表示不加密)
    std::unique_ptr<IEncryptionStrategy> encryptor_;
    // 备份阶段选择的压缩策略(为空表示不压缩)
    std::unique_ptr<ICompressionStrategy> compressor_;
};

} // namespace DataBackup