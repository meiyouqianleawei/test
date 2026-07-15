#include "BackupWorker.h"

namespace DataBackup {

BackupWorker::BackupWorker(QObject* parent)
    : QThread(parent)
    , backupEngine_(std::make_unique<BackupEngine>())
    , cancelled_(false)
{
}

BackupWorker::~BackupWorker() {
    if (isRunning()) {
        cancel();
        wait();
    }
}

void BackupWorker::setConfig(const BackupConfig& config) {
    config_ = config;
}

void BackupWorker::cancel() {
    cancelled_ = true;
    if (backupEngine_) {
        backupEngine_->cancelBackup();
    }
}

bool BackupWorker::isRunning() const {
    return QThread::isRunning();
}

void BackupWorker::run() {
    cancelled_ = false;

    // 设置进度回调
    backupEngine_->setProgressCallback([this](const BackupProgress& progress) {
        onProgress(progress);
    });

    // 执行备份
    BackupResult result = backupEngine_->createBackup(config_);

    // 发送完成信号（使用 Qt::QueuedConnection 确保线程安全）
    emit backupComplete(result);
}

void BackupWorker::onProgress(const BackupProgress& progress) {
    // 发送进度更新信号（使用 Qt::QueuedConnection 确保线程安全）
    emit progressUpdate(progress);
}

} // namespace DataBackup