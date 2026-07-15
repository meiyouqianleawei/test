#pragma once

#include <QThread>
#include <memory>
#include <string>
#include "engine/RestoreEngine.h"
#include "engine/BackupTypes.h"

namespace DataBackup {

/**
 * 还原工作线程类
 * 在后台线程执行还原任务，通过信号槽与主线程通信
 */
class RestoreWorker : public QThread {
    Q_OBJECT

public:
    /**
     * 构造函数
     * @param parent 父对象
     */
    explicit RestoreWorker(QObject* parent = nullptr);

    /**
     * 析构函数
     */
    ~RestoreWorker() override;

    /**
     * 还原模式
     */
    enum class Mode {
        Package,  // 从 .pkg 打包文件还原
        Folder    // 从实时备份目录还原（可选解密）
    };

    /**
     * 设置备份文件路径
     * @param filePath 备份文件路径
     */
    void setBackupFile(const std::string& filePath);

    /**
     * 设置还原模式
     */
    void setMode(Mode mode);

    /**
     * 设置目标还原目录
     * @param directory 目标目录
     */
    void setTargetDirectory(const std::string& directory);

    /**
     * 设置解密密码
     * @param password 密码
     */
    void setPassword(const std::string& password);

    /**
     * 取消还原
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
     * 还原完成信号
     * @param result 还原结果
     */
    void restoreComplete(const RestoreResult& result);

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
    // 还原实时备份目录（复制/解密源文件夹到目标文件夹）
    RestoreResult restoreFolder();

private:
    std::string backupFilePath_;               // 备份文件/目录路径
    std::string targetDirectory_;              // 目标目录
    std::string password_;                     // 解密密码
    Mode mode_ = Mode::Package;                // 还原模式

    std::unique_ptr<RestoreEngine> restoreEngine_; // 还原引擎
    std::atomic<bool> cancelled_;               // 取消标志
};

} // namespace DataBackup