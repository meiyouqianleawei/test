#include "MainWindow.h"
#include <QMenuBar>
#include <QStatusBar>
#include <QMessageBox>
#include <QApplication>

namespace DataBackup {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , tabWidget_(nullptr)
    , backupTab_(nullptr)
    , restoreTab_(nullptr)
    , schedulerTab_(nullptr)
    , realtimeTab_(nullptr)
{
    setupUI();
    createMenuBar();
    createStatusBar();
}

MainWindow::~MainWindow() {
    // Qt 会自动删除子对象
}

void MainWindow::setupUI() {
    // 创建选项卡控件
    tabWidget_ = new QTabWidget(this);
    tabWidget_->setTabPosition(QTabWidget::North);
    tabWidget_->setDocumentMode(true);

    // 创建备份选项卡
    backupTab_ = new BackupTab(this);
    tabWidget_->addTab(backupTab_, "备份");

    // 创建还原选项卡
    restoreTab_ = new RestoreTab(this);
    tabWidget_->addTab(restoreTab_, "还原");

    // 创建定时任务选项卡
    schedulerTab_ = new SchedulerTab(this);
    tabWidget_->addTab(schedulerTab_, "定时任务");

    // 创建实时备份选项卡
    realtimeTab_ = new RealtimeTab(this);
    tabWidget_->addTab(realtimeTab_, "实时备份");

    // 设置为中心控件
    setCentralWidget(tabWidget_);
}

void MainWindow::createMenuBar() {
    // 创建菜单栏
    QMenuBar* menuBar = this->menuBar();

    // 文件菜单
    QMenu* fileMenu = menuBar->addMenu("文件");

    QAction* exitAction = new QAction("退出", this);
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QMainWindow::close);
    fileMenu->addAction(exitAction);

    // 帮助菜单
    QMenu* helpMenu = menuBar->addMenu("帮助");

    QAction* aboutAction = new QAction("关于", this);
    connect(aboutAction, &QAction::triggered, [this]() {
        QMessageBox::about(this, "关于",
            "数据备份软件 v1.0.0\n\n"
            "功能特性:\n"
            "- 文件备份与还原\n"
            "- 支持压缩和加密\n"
            "- 定时备份任务\n"
            "- 实时文件监控\n\n"
            "© 2024 DataBackup Team");
    });
    helpMenu->addAction(aboutAction);
}

void MainWindow::createStatusBar() {
    statusBar()->showMessage("就绪");
}

} // namespace DataBackup