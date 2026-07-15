#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QProgressBar>
#include <QLabel>
#include <QRadioButton>
#include <memory>
#include "RestoreWorker.h"

namespace DataBackup {

/**
 * 还原功能选项卡
 * 提供用户界面进行备份文件还原操作
 */
class RestoreTab : public QWidget {
    Q_OBJECT

public:
    /**
     * 构造函数
     * @param parent 父窗口
     */
    explicit RestoreTab(QWidget* parent = nullptr);

    /**
     * 析构函数
     */
    ~RestoreTab() override;

private slots:
    /**
     * 选择备份文件/目录（根据模式）
     */
    void selectBackupFile();

    /**
     * 选择还原目录
     */
    void selectRestoreDirectory();

    /**
     * 还原模式切换
     */
    void onModeChanged();

    /**
     * 开始还原
     */
    void startRestore();

    /**
     * 取消还原
     */
    void cancelRestore();

    /**
     * 更新还原进度
     * @param progress 进度信息
     */
    void onProgressUpdate(const BackupProgress& progress);

    /**
     * 还原完成
     * @param result 还原结果
     */
    void onRestoreComplete(const RestoreResult& result);

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
    // 模式选择
    QRadioButton* modePackageRadio_;
    QRadioButton* modeFolderRadio_;

    // 备份文件选择
    QLabel* backupFileLabel_;
    QLineEdit* backupFileEdit_;
    QPushButton* backupFileButton_;

    // 还原目录选择
    QLabel* restoreDirLabel_;
    QLineEdit* restoreDirEdit_;
    QPushButton* restoreDirButton_;

    // 密码输入
    QLabel* passwordLabel_;
    QLineEdit* passwordEdit_;

    // 控制按钮
    QPushButton* startButton_;
    QPushButton* cancelButton_;

    // 进度显示
    QProgressBar* progressBar_;
    QLabel* statusLabel_;

    // 还原工作线程
    RestoreWorker* restoreWorker_;
};

} // namespace DataBackup