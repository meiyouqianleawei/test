#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QLabel>
#include <memory>
#include "RealtimeWorker.h"

namespace DataBackup {

/**
 * 实时备份选项卡
 * 提供用户界面进行实时文件监控和备份
 */
class RealtimeTab : public QWidget {
    Q_OBJECT

public:
    /**
     * 构造函数
     * @param parent 父窗口
     */
    explicit RealtimeTab(QWidget* parent = nullptr);

    /**
     * 析构函数
     */
    ~RealtimeTab() override;

private slots:
    /**
     * 选择监控目录
     */
    void selectMonitorDirectory();

    /**
     * 选择备份目标
     */
    void selectBackupTarget();

    /**
     * 开始监控
     */
    void startMonitor();

    /**
     * 停止监控
     */
    void stopMonitor();

    /**
     * 暂停监控
     */
    void pauseMonitor();

    /**
     * 恢复监控
     */
    void resumeMonitor();

    /**
     * 加密选项改变
     * @param checked 是否选中
     */
    void onEncryptionChanged(bool checked);

    /**
     * 监控状态更新
     * @param status 状态信息
     */
    void onStatusUpdate(const QString& status);

    /**
     * 统计信息更新
     * @param stats 统计信息
     */
    void onStatsUpdate(const RealtimeBackupStats& stats);

private:
    /**
     * 初始化界面
     */
    void setupUI();

    /**
     * 创建连接
     */
    void createConnections();

    /**
     * 更新按钮状态
     * @param isRunning 是否正在运行
     */
    void updateButtonState(bool isRunning);

private:
    // 监控目录选择
    QLabel* monitorDirLabel_;
    QLineEdit* monitorDirEdit_;
    QPushButton* monitorDirButton_;

    // 备份目标选择
    QLabel* backupTargetLabel_;
    QLineEdit* backupTargetEdit_;
    QPushButton* backupTargetButton_;

    // 加密选项
    QCheckBox* encryptionCheck_;
    QLabel* passwordLabel_;
    QLineEdit* passwordEdit_;

    // 文件过滤器
    QLabel* filterLabel_;
    QLineEdit* filterEdit_;

    // 行为选项
    QCheckBox* subdirCheck_;
    QCheckBox* initialSyncCheck_;
    QCheckBox* mirrorDeleteCheck_;

    // 控制按钮
    QPushButton* startButton_;
    QPushButton* stopButton_;
    QPushButton* pauseButton_;
    QPushButton* resumeButton_;

    // 状态显示
    QLabel* statusLabel_;
    QLabel* filesBackedUpLabel_;
    QLabel* filesDeletedLabel_;
    QLabel* totalBytesLabel_;
    QLabel* backupFailuresLabel_;

    // 实时备份工作类
    RealtimeWorker* realtimeWorker_;
};

} // namespace DataBackup