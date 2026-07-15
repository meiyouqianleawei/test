#pragma once

#include <QThread>
#include <memory>
#include "engine/BackupEngine.h"
#include "engine/BackupConfig.h"
#include "engine/BackupTypes.h"

namespace DataBackup {

/**
 * 备份工作线程类
 * 在后台线程执行备份任务，通过信号槽与主线程通信
 */
class BackupWorker : public QThread {
    Q_OBJECT

public:
    /**
     * 构造函数
     * @param parent 父对象
     */
    explicit BackupWorker(QObject* parent = nullptr);

    /**
     * 析构函数
     */
    ~BackupWorker() override;

    /**
     * 设置备份配置
     * @param config 备份配置
     */
    void setConfig(const BackupConfig& config);

    /**
     * 取消备份
     */
    void cancel();

    /**
     * 检查是否正在运行
     * @return 是否正在运行
     */
    bool isRunning() const;

signals:
    /**
     * 进度更新信号
     * @param progress 进度信息
     */
    void progressUpdate(const BackupProgress& progress);

    /**
     * 备份完成信号
     * @param result 备份结果
     */
    void backupComplete(const BackupResult& result);

protected:
    /**
     * 线程主函数
     */
    void run() override;

private:
    /**
     * 进度回调函数
     * @param progress 进度信息
     */
    void onProgress(const BackupProgress& progress);

private:
    BackupConfig config_;                       // 备份配置
    std::unique_ptr<BackupEngine> backupEngine_; // 备份引擎
    std::atomic<bool> cancelled_;               // 取消标志
};

} // namespace DataBackup