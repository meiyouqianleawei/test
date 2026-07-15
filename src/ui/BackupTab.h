#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QCheckBox>
#include <QProgressBar>
#include <QLabel>
#include <memory>
#include "BackupWorker.h"
#include "FilterDialog.h"

namespace DataBackup {

/**
 * 备份功能选项卡
 * 提供用户界面进行文件备份操作
 */
class BackupTab : public QWidget {
    Q_OBJECT

public:
    /**
     * 构造函数
     * @param parent 父窗口
     */
    explicit BackupTab(QWidget* parent = nullptr);

    /**
     * 析构函数
     */
    ~BackupTab() override;

private slots:
    /**
     * 选择源目录
     */
    void selectSourceDirectory();

    /**
     * 选择目标路径
     */
    void selectTargetPath();

    /**
     * 开始备份
     */
    void startBackup();

    /**
     * 取消备份
     */
    void cancelBackup();

    /**
     * 更新备份进度
     * @param progress 进度信息
     */
    void onProgressUpdate(const BackupProgress& progress);

    /**
     * 备份完成
     * @param result 备份结果
     */
    void onBackupComplete(const BackupResult& result);

    /**
     * 加密选项改变
     * @param checked 是否选中
     */
    void onEncryptionChanged(bool checked);

    /**
     * 打开文件过滤设置对话框
     */
    void openFilterDialog();

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
    // 源目录选择
    QLabel* sourceLabel_;
    QLineEdit* sourceEdit_;
    QPushButton* sourceButton_;

    // 目标路径选择
    QLabel* targetLabel_;
    QLineEdit* targetEdit_;
    QPushButton* targetButton_;

    // 压缩选项
    QLabel* compressionLabel_;
    QComboBox* compressionCombo_;

    // 加密选项
    QCheckBox* encryptionCheck_;
    QLabel* passwordLabel_;
    QLineEdit* passwordEdit_;

    // 过滤选项
    QPushButton* filterButton_;
    QLabel* filterStatusLabel_;
    FilterOptions filterOptions_;

    // 控制按钮
    QPushButton* startButton_;
    QPushButton* cancelButton_;

    // 进度显示
    QProgressBar* progressBar_;
    QLabel* statusLabel_;

    // 备份工作线程
    BackupWorker* backupWorker_;
};

} // namespace DataBackup