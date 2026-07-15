#include "RealtimeWorker.h"
#include <QTimer>

namespace DataBackup {

RealtimeWorker::RealtimeWorker(QObject* parent)
    : QObject(parent)
    , monitor_(std::make_unique<RealtimeBackupMonitor>())
    , statsTimer_(new QTimer(this))
{
    // 设置定时器，每秒更新一次统计信息
    connect(statsTimer_, &QTimer::timeout, this, &RealtimeWorker::updateStats);
}

RealtimeWorker::~RealtimeWorker() {
    stop();
}

bool RealtimeWorker::start(const RealtimeBackupConfig& config) {
    if (!monitor_) {
        return false;
    }

    // 设置文件变化回调
    bool result = monitor_->startMonitor(config, [this](const FileChangeEvent& event) {
        QString statusMsg;
        switch (event.type) {
            case FileChangeType::CREATED:
                statusMsg = QString("文件创建: %1").arg(QString::fromStdString(event.filePath));
                break;
            case FileChangeType::MODIFIED:
                statusMsg = QString("文件修改: %1").arg(QString::fromStdString(event.filePath));
                break;
            case FileChangeType::DELETED:
                statusMsg = QString("文件删除: %1").arg(QString::fromStdString(event.filePath));
                break;
            case FileChangeType::RENAMED:
                statusMsg = QString("文件重命名: %1 -> %2")
                    .arg(QString::fromStdString(event.oldFilePath))
                    .arg(QString::fromStdString(event.filePath));
                break;
        }
        emit statusUpdate(statusMsg);
    });

    if (result) {
        statsTimer_->start(1000); // 每秒更新统计信息
    }

    return result;
}

void RealtimeWorker::stop() {
    if (monitor_) {
        monitor_->stopMonitor();
    }

    if (statsTimer_) {
        statsTimer_->stop();
    }
}

void RealtimeWorker::pause() {
    if (monitor_) {
        monitor_->pauseMonitor();
        emit statusUpdate("已暂停");
    }
}

void RealtimeWorker::resume() {
    if (monitor_) {
        monitor_->resumeMonitor();
        emit statusUpdate("运行中");
    }
}

bool RealtimeWorker::isRunning() const {
    if (!monitor_) {
        return false;
    }

    return monitor_->isRunning();
}

void RealtimeWorker::updateStats() {
    if (monitor_) {
        RealtimeBackupStats stats = monitor_->getStats();
        emit statsUpdate(stats);
    }
}

} // namespace DataBackup