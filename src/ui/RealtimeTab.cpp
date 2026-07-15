#include "RealtimeTab.h"
#include <QFileDialog>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QRegularExpression>

namespace DataBackup {

RealtimeTab::RealtimeTab(QWidget* parent)
    : QWidget(parent)
    , monitorDirLabel_(nullptr)
    , monitorDirEdit_(nullptr)
    , monitorDirButton_(nullptr)
    , backupTargetLabel_(nullptr)
    , backupTargetEdit_(nullptr)
    , backupTargetButton_(nullptr)
    , encryptionCheck_(nullptr)
    , passwordLabel_(nullptr)
    , passwordEdit_(nullptr)
    , filterLabel_(nullptr)
    , filterEdit_(nullptr)
    , subdirCheck_(nullptr)
    , initialSyncCheck_(nullptr)
    , mirrorDeleteCheck_(nullptr)
    , startButton_(nullptr)
    , stopButton_(nullptr)
    , pauseButton_(nullptr)
    , resumeButton_(nullptr)
    , statusLabel_(nullptr)
    , filesBackedUpLabel_(nullptr)
    , filesDeletedLabel_(nullptr)
    , totalBytesLabel_(nullptr)
    , backupFailuresLabel_(nullptr)
    , realtimeWorker_(nullptr)
{
    setupUI();
    createConnections();

    // 初始化实时备份工作类
    realtimeWorker_ = new RealtimeWorker(this);
    connect(realtimeWorker_, &RealtimeWorker::statusUpdate,
            this, &RealtimeTab::onStatusUpdate, Qt::QueuedConnection);
    connect(realtimeWorker_, &RealtimeWorker::statsUpdate,
            this, &RealtimeTab::onStatsUpdate, Qt::QueuedConnection);
}

RealtimeTab::~RealtimeTab() {
    if (realtimeWorker_) {
        realtimeWorker_->stop();
    }
}

void RealtimeTab::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // 说明文字
    QLabel* hintLabel = new QLabel(
        "实时备份：把源目录的变化持续同步到目标目录（文件夹级镜像，不打包）。\n"
        "可与「普通备份」「定时备份」在同一源目录并行使用；目标目录不能位于源目录内部。",
        this);
    hintLabel->setStyleSheet("color:#555;");
    hintLabel->setWordWrap(true);
    mainLayout->addWidget(hintLabel);

    // 配置区域
    QGroupBox* configGroup = new QGroupBox("监控配置", this);
    QGridLayout* configLayout = new QGridLayout(configGroup);

    int row = 0;

    // 监控目录
    monitorDirLabel_ = new QLabel("监控目录:", this);
    monitorDirEdit_ = new QLineEdit(this);
    monitorDirButton_ = new QPushButton("浏览...", this);

    configLayout->addWidget(monitorDirLabel_, row, 0);
    configLayout->addWidget(monitorDirEdit_, row, 1);
    configLayout->addWidget(monitorDirButton_, row, 2);
    row++;

    // 备份目标
    backupTargetLabel_ = new QLabel("备份目标:", this);
    backupTargetEdit_ = new QLineEdit(this);
    backupTargetButton_ = new QPushButton("浏览...", this);

    configLayout->addWidget(backupTargetLabel_, row, 0);
    configLayout->addWidget(backupTargetEdit_, row, 1);
    configLayout->addWidget(backupTargetButton_, row, 2);
    row++;

    // 加密选项
    encryptionCheck_ = new QCheckBox("启用加密", this);
    passwordEdit_ = new QLineEdit(this);
    passwordEdit_->setEchoMode(QLineEdit::Password);
    passwordEdit_->setPlaceholderText("加密密码");
    passwordEdit_->setEnabled(false);
    passwordLabel_ = new QLabel("密码:", this);

    configLayout->addWidget(encryptionCheck_, row, 0);
    configLayout->addWidget(passwordLabel_, row, 1);
    configLayout->addWidget(passwordEdit_, row, 2);
    row++;

    // 文件过滤器
    filterLabel_ = new QLabel("扩展名过滤:", this);
    filterEdit_ = new QLineEdit(this);
    filterEdit_->setPlaceholderText("例如: txt doc pdf  (空格分隔，留空监控全部)");

    configLayout->addWidget(filterLabel_, row, 0);
    configLayout->addWidget(filterEdit_, row, 1, 1, 2);
    row++;

    // 行为选项
    subdirCheck_ = new QCheckBox("监控子目录", this);
    subdirCheck_->setChecked(true);
    initialSyncCheck_ = new QCheckBox("启动时全量同步", this);
    initialSyncCheck_->setChecked(true);
    mirrorDeleteCheck_ = new QCheckBox("同步删除/重命名", this);
    mirrorDeleteCheck_->setChecked(true);

    QHBoxLayout* optionsLayout = new QHBoxLayout();
    optionsLayout->addWidget(subdirCheck_);
    optionsLayout->addWidget(initialSyncCheck_);
    optionsLayout->addWidget(mirrorDeleteCheck_);
    optionsLayout->addStretch();
    configLayout->addLayout(optionsLayout, row, 0, 1, 3);
    row++;

    mainLayout->addWidget(configGroup);

    // 控制按钮
    QHBoxLayout* buttonLayout = new QHBoxLayout();

    startButton_ = new QPushButton("开始监控", this);
    stopButton_ = new QPushButton("停止监控", this);
    stopButton_->setEnabled(false);
    pauseButton_ = new QPushButton("暂停", this);
    pauseButton_->setEnabled(false);
    resumeButton_ = new QPushButton("恢复", this);
    resumeButton_->setEnabled(false);

    buttonLayout->addWidget(startButton_);
    buttonLayout->addWidget(stopButton_);
    buttonLayout->addWidget(pauseButton_);
    buttonLayout->addWidget(resumeButton_);
    buttonLayout->addStretch();

    mainLayout->addLayout(buttonLayout);

    // 状态显示
    QGroupBox* statusGroup = new QGroupBox("监控状态", this);
    QVBoxLayout* statusLayout = new QVBoxLayout(statusGroup);

    statusLabel_ = new QLabel("状态: 未启动", this);
    filesBackedUpLabel_ = new QLabel("已备份文件数: 0", this);
    filesDeletedLabel_ = new QLabel("已同步删除数: 0", this);
    totalBytesLabel_ = new QLabel("总备份字节数: 0", this);
    backupFailuresLabel_ = new QLabel("备份失败次数: 0", this);

    statusLayout->addWidget(statusLabel_);
    statusLayout->addWidget(filesBackedUpLabel_);
    statusLayout->addWidget(filesDeletedLabel_);
    statusLayout->addWidget(totalBytesLabel_);
    statusLayout->addWidget(backupFailuresLabel_);

    mainLayout->addWidget(statusGroup);
    mainLayout->addStretch();
}

void RealtimeTab::createConnections() {
    connect(monitorDirButton_, &QPushButton::clicked, this, &RealtimeTab::selectMonitorDirectory);
    connect(backupTargetButton_, &QPushButton::clicked, this, &RealtimeTab::selectBackupTarget);
    connect(startButton_, &QPushButton::clicked, this, &RealtimeTab::startMonitor);
    connect(stopButton_, &QPushButton::clicked, this, &RealtimeTab::stopMonitor);
    connect(pauseButton_, &QPushButton::clicked, this, &RealtimeTab::pauseMonitor);
    connect(resumeButton_, &QPushButton::clicked, this, &RealtimeTab::resumeMonitor);
    connect(encryptionCheck_, &QCheckBox::toggled, this, &RealtimeTab::onEncryptionChanged);
}

void RealtimeTab::selectMonitorDirectory() {
    QString dir = QFileDialog::getExistingDirectory(this, "选择监控目录",
        monitorDirEdit_->text(), QFileDialog::ShowDirsOnly);
    if (!dir.isEmpty()) {
        monitorDirEdit_->setText(dir);
    }
}

void RealtimeTab::selectBackupTarget() {
    QString dir = QFileDialog::getExistingDirectory(this, "选择备份目标目录",
        backupTargetEdit_->text(), QFileDialog::ShowDirsOnly);
    if (!dir.isEmpty()) {
        backupTargetEdit_->setText(dir);
    }
}

void RealtimeTab::startMonitor() {
    // 验证输入
    if (monitorDirEdit_->text().isEmpty()) {
        QMessageBox::warning(this, "警告", "请选择监控目录");
        return;
    }

    if (backupTargetEdit_->text().isEmpty()) {
        QMessageBox::warning(this, "警告", "请选择备份目标目录");
        return;
    }

    if (encryptionCheck_->isChecked() && passwordEdit_->text().isEmpty()) {
        QMessageBox::warning(this, "警告", "请输入加密密码");
        return;
    }

    // 准备配置
    RealtimeBackupConfig config;
    config.sourcePath = monitorDirEdit_->text().toStdString();
    config.targetPath = backupTargetEdit_->text().toStdString();
    config.enableEncryption = encryptionCheck_->isChecked();
    config.encryptionPassword = passwordEdit_->text().toStdString();
    config.includeSubdirectories = subdirCheck_->isChecked();
    config.initialFullSync = initialSyncCheck_->isChecked();
    config.mirrorDelete = mirrorDeleteCheck_->isChecked();

    // 解析扩展名过滤器（空格分隔，兼容 "*.txt"、".txt"、"txt"）
    if (!filterEdit_->text().trimmed().isEmpty()) {
        QStringList filters = filterEdit_->text().split(
            QRegularExpression("[\\s,;]+"), Qt::SkipEmptyParts);
        for (const QString& filter : filters) {
            config.fileFilters.push_back(filter.toStdString());
        }
    }

    if (realtimeWorker_->start(config)) {
        updateButtonState(true);
        statusLabel_->setText("状态: 运行中");
    } else {
        QMessageBox::critical(this, "错误",
            "启动监控失败。请检查源目录是否存在、目标目录是否可写，"
            "且目标目录不能等于或位于源目录内部。");
    }
}

void RealtimeTab::stopMonitor() {
    realtimeWorker_->stop();
    updateButtonState(false);
    statusLabel_->setText("状态: 已停止");
}

void RealtimeTab::pauseMonitor() {
    realtimeWorker_->pause();
    statusLabel_->setText("状态: 已暂停");
    pauseButton_->setEnabled(false);
    resumeButton_->setEnabled(true);
}

void RealtimeTab::resumeMonitor() {
    realtimeWorker_->resume();
    statusLabel_->setText("状态: 运行中");
    pauseButton_->setEnabled(true);
    resumeButton_->setEnabled(false);
}

void RealtimeTab::onEncryptionChanged(bool checked) {
    passwordEdit_->setEnabled(checked);
    if (!checked) {
        passwordEdit_->clear();
    }
}

void RealtimeTab::onStatusUpdate(const QString& status) {
    statusLabel_->setText(QString("状态: %1").arg(status));
}

void RealtimeTab::onStatsUpdate(const RealtimeBackupStats& stats) {
    filesBackedUpLabel_->setText(QString("已备份文件数: %1").arg(stats.filesBackedUp));
    filesDeletedLabel_->setText(QString("已同步删除数: %1").arg(stats.filesDeleted));
    totalBytesLabel_->setText(QString("总备份字节数: %1").arg(stats.totalBytesBackedUp));
    backupFailuresLabel_->setText(QString("备份失败次数: %1").arg(stats.backupFailures));
}

void RealtimeTab::updateButtonState(bool isRunning) {
    startButton_->setEnabled(!isRunning);
    stopButton_->setEnabled(isRunning);
    pauseButton_->setEnabled(isRunning);
    resumeButton_->setEnabled(false);

    monitorDirEdit_->setEnabled(!isRunning);
    backupTargetEdit_->setEnabled(!isRunning);
    monitorDirButton_->setEnabled(!isRunning);
    backupTargetButton_->setEnabled(!isRunning);
    encryptionCheck_->setEnabled(!isRunning);
    passwordEdit_->setEnabled(!isRunning && encryptionCheck_->isChecked());
    filterEdit_->setEnabled(!isRunning);
    subdirCheck_->setEnabled(!isRunning);
    initialSyncCheck_->setEnabled(!isRunning);
    mirrorDeleteCheck_->setEnabled(!isRunning);
}

} // namespace DataBackup
