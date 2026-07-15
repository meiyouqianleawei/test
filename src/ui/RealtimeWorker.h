#pragma once

#include <QObject>
#include <memory>
#include "realtime/RealtimeBackupMonitor.h"
#include "realtime/RealtimeBackupTypes.h"

namespace DataBackup {

/**
 * 实时备份工作类
 * 封装 RealtimeBackupMonitor，管理实时备份监控
 */
class RealtimeWorker : public QObject {
    Q_OBJECT

public:
    /**
     * 构造函数
     * @param parent 父对象
     */
    explicit RealtimeWorker(QObject* parent = nullptr);

    /**
     * 析构函数
     */
    ~RealtimeWorker() override;

    /**
     * 启动监控
     * @param config 监控配置
     * @return 是否成功
     */
    bool start(const RealtimeBackupConfig& config);

    /**
     * 停止监控
     */
    void stop();

    /**
     * 暂停监控
     */
    void pause();

    /**
     * 恢复监控
     */
    void resume();

    /**
     * 检查是否正在运行
     * @return 是否正在运行
     */
    bool isRunning() const;

signals:
    /**
     * 状态更新信号
     * @param status 状态信息
     */
    void statusUpdate(const QString& status);

    /**
     * 统计信息更新信号
     * @param stats 统计信息
     */
    void statsUpdate(const RealtimeBackupStats& stats);

private:
    /**
     * 定时器回调，定期更新统计信息
     */
    void updateStats();

private:
    std::unique_ptr<RealtimeBackupMonitor> monitor_; // 实时备份监控器
    QTimer* statsTimer_;                             // 统计信息更新定时器
};

} // namespace DataBackup