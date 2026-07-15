#pragma once

#include <QMainWindow>
#include <QTabWidget>
#include <memory>
#include "BackupTab.h"
#include "RestoreTab.h"
#include "SchedulerTab.h"
#include "RealtimeTab.h"

namespace DataBackup {

/**
 * 主窗口类
 * 使用选项卡布局管理所有功能
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    /**
     * 构造函数
     * @param parent 父窗口
     */
    explicit MainWindow(QWidget* parent = nullptr);

    /**
     * 析构函数
     */
    ~MainWindow() override;

private:
    /**
     * 初始化界面
     */
    void setupUI();

    /**
     * 创建菜单栏
     */
    void createMenuBar();

    /**
     * 创建状态栏
     */
    void createStatusBar();

private:
    // 选项卡控件
    QTabWidget* tabWidget_;

    // 功能选项卡
    BackupTab* backupTab_;
    RestoreTab* restoreTab_;
    SchedulerTab* schedulerTab_;
    RealtimeTab* realtimeTab_;
};

} // namespace DataBackup