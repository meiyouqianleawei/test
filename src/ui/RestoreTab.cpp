#include "RestoreTab.h"
#include <QFileDialog>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QMessageBox>
#include <QButtonGroup>
#include <QGroupBox>

namespace DataBackup {

RestoreTab::RestoreTab(QWidget* parent)
    : QWidget(parent)
    , modePackageRadio_(nullptr)
    , modeFolderRadio_(nullptr)
    , backupFileLabel_(nullptr)
    , backupFileEdit_(nullptr)
    , backupFileButton_(nullptr)
    , restoreDirLabel_(nullptr)
    , restoreDirEdit_(nullptr)
    , restoreDirButton_(nullptr)
    , passwordLabel_(nullptr)
    , passwordEdit_(nullptr)
    , startButton_(nullptr)
    , cancelButton_(nullptr)
    , progressBar_(nullptr)
    , statusLabel_(nullptr)
    , restoreWorker_(nullptr)
{
    setupUI();
    createConnections();
    onModeChanged();
}

RestoreTab::~RestoreTab() {
    if (restoreWorker_ && restoreWorker_->isRunning()) {
        restoreWorker_->cancel();
        restoreWorker_->wait();
    }
}

void RestoreTab::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // 还原模式选择
    QGroupBox* modeGroup = new QGroupBox("还原来源", this);
    QHBoxLayout* modeLayout = new QHBoxLayout(modeGroup);
    modePackageRadio_ = new QRadioButton("从打包文件还原 (.pkg)", this);
    modeFolderRadio_  = new QRadioButton("从实时备份目录还原（文件夹）", this);
    modePackageRadio_->setChecked(true);
    QButtonGroup* modeBtnGroup = new QButtonGroup(this);
    modeBtnGroup->addButton(modePackageRadio_);
    modeBtnGroup->addButton(modeFolderRadio_);
    modeLayout->addWidget(modePackageRadio_);
    modeLayout->addWidget(modeFolderRadio_);
    modeLayout->addStretch();
    mainLayout->addWidget(modeGroup);

    // 备份文件/目录 选择
    QGridLayout* gridLayout = new QGridLayout();

    backupFileLabel_ = new QLabel("备份文件:", this);
    backupFileEdit_ = new QLineEdit(this);
    backupFileButton_ = new QPushButton("浏览...", this);

    gridLayout->addWidget(backupFileLabel_, 0, 0);
    gridLayout->addWidget(backupFileEdit_, 0, 1);
    gridLayout->addWidget(backupFileButton_, 0, 2);

    // 还原目录选择
    restoreDirLabel_ = new QLabel("还原目录:", this);
    restoreDirEdit_ = new QLineEdit(this);
    restoreDirButton_ = new QPushButton("浏览...", this);

    gridLayout->addWidget(restoreDirLabel_, 1, 0);
    gridLayout->addWidget(restoreDirEdit_, 1, 1);
    gridLayout->addWidget(restoreDirButton_, 1, 2);

    // 密码输入
    passwordLabel_ = new QLabel("解密密码:", this);
    passwordEdit_ = new QLineEdit(this);
    passwordEdit_->setEchoMode(QLineEdit::Password);
    passwordEdit_->setPlaceholderText("如未加密可留空");

    gridLayout->addWidget(passwordLabel_, 2, 0);
    gridLayout->addWidget(passwordEdit_, 2, 1);

    mainLayout->addLayout(gridLayout);

    // 控制按钮
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    startButton_ = new QPushButton("开始还原", this);
    cancelButton_ = new QPushButton("取消", this);
    cancelButton_->setEnabled(false);

    buttonLayout->addStretch();
    buttonLayout->addWidget(startButton_);
    buttonLayout->addWidget(cancelButton_);

    mainLayout->addLayout(buttonLayout);

    // 进度显示
    progressBar_ = new QProgressBar(this);
    progressBar_->setRange(0, 100);
    progressBar_->setValue(0);
    progressBar_->setTextVisible(true);

    statusLabel_ = new QLabel("就绪", this);

    mainLayout->addWidget(progressBar_);
    mainLayout->addWidget(statusLabel_);
    mainLayout->addStretch();
}

void RestoreTab::createConnections() {
    connect(backupFileButton_, &QPushButton::clicked, this, &RestoreTab::selectBackupFile);
    connect(restoreDirButton_, &QPushButton::clicked, this, &RestoreTab::selectRestoreDirectory);
    connect(startButton_, &QPushButton::clicked, this, &RestoreTab::startRestore);
    connect(cancelButton_, &QPushButton::clicked, this, &RestoreTab::cancelRestore);
    connect(modePackageRadio_, &QRadioButton::toggled, this, &RestoreTab::onModeChanged);
    connect(modeFolderRadio_,  &QRadioButton::toggled, this, &RestoreTab::onModeChanged);
}

void RestoreTab::onModeChanged() {
    bool folderMode = modeFolderRadio_->isChecked();
    backupFileLabel_->setText(folderMode ? "备份目录:" : "备份文件:");
    passwordEdit_->setPlaceholderText(
        folderMode
            ? "若备份时启用了加密请输入密码，否则留空"
            : "如未加密可留空");
    backupFileEdit_->clear();
}

void RestoreTab::selectBackupFile() {
    if (modeFolderRadio_->isChecked()) {
        QString dir = QFileDialog::getExistingDirectory(this, "选择备份目录（实时备份的目标目录）",
            backupFileEdit_->text(), QFileDialog::ShowDirsOnly);
        if (!dir.isEmpty()) {
            backupFileEdit_->setText(dir);
        }
    } else {
        QString file = QFileDialog::getOpenFileName(this, "选择备份文件",
            backupFileEdit_->text(),
            "备份文件 (*.pkg *.backup);;打包文件 (*.pkg);;备份文件 (*.backup);;所有文件 (*)");
        if (!file.isEmpty()) {
            backupFileEdit_->setText(file);
        }
    }
}

void RestoreTab::selectRestoreDirectory() {
    QString dir = QFileDialog::getExistingDirectory(this, "选择还原目录",
        restoreDirEdit_->text(), QFileDialog::ShowDirsOnly);
    if (!dir.isEmpty()) {
        restoreDirEdit_->setText(dir);
    }
}

void RestoreTab::startRestore() {
    // 验证输入
    if (backupFileEdit_->text().isEmpty()) {
        QMessageBox::warning(this, "警告", "请选择备份文件");
        return;
    }

    if (restoreDirEdit_->text().isEmpty()) {
        QMessageBox::warning(this, "警告", "请选择还原目录");
        return;
    }

    // 创建并启动工作线程
    if (!restoreWorker_) {
        restoreWorker_ = new RestoreWorker(this);
        connect(restoreWorker_, &RestoreWorker::progressUpdate,
                this, &RestoreTab::onProgressUpdate, Qt::QueuedConnection);
        connect(restoreWorker_, &RestoreWorker::restoreComplete,
                this, &RestoreTab::onRestoreComplete, Qt::QueuedConnection);
    }

    restoreWorker_->setBackupFile(backupFileEdit_->text().toStdString());
    restoreWorker_->setTargetDirectory(restoreDirEdit_->text().toStdString());
    restoreWorker_->setPassword(passwordEdit_->text().toStdString());
    restoreWorker_->setMode(modeFolderRadio_->isChecked()
                            ? RestoreWorker::Mode::Folder
                            : RestoreWorker::Mode::Package);
    restoreWorker_->start();

    updateButtonState(true);
    statusLabel_->setText("正在还原...");
}

void RestoreTab::cancelRestore() {
    if (restoreWorker_ && restoreWorker_->isRunning()) {
        restoreWorker_->cancel();
        statusLabel_->setText("正在取消...");
    }
}

void RestoreTab::onProgressUpdate(const BackupProgress& progress) {
    progressBar_->setValue(progress.percentage);
    statusLabel_->setText(QString::fromStdString(progress.getPhaseName()) + 
                         " - " + QString::fromStdString(progress.currentFile));
}

void RestoreTab::onRestoreComplete(const RestoreResult& result) {
    updateButtonState(false);

    if (result.success) {
        progressBar_->setValue(100);
        statusLabel_->setText(QString("还原完成 - 已还原 %1 个文件，总大小 %2 字节")
            .arg(result.filesRestored)
            .arg(result.totalSize));

        QMessageBox::information(this, "成功",
            QString("还原完成!\n\n"
                    "还原文件数: %1\n"
                    "还原大小: %2 字节\n"
                    "耗时: %3 毫秒")
            .arg(result.filesRestored)
            .arg(result.totalSize)
            .arg(result.duration));
    } else {
        statusLabel_->setText("还原失败");
        QMessageBox::critical(this, "错误",
            QString("还原失败:\n%1").arg(QString::fromStdString(result.errorMessage)));
    }
}

void RestoreTab::updateButtonState(bool isRunning) {
    startButton_->setEnabled(!isRunning);
    cancelButton_->setEnabled(isRunning);
    backupFileEdit_->setEnabled(!isRunning);
    restoreDirEdit_->setEnabled(!isRunning);
    backupFileButton_->setEnabled(!isRunning);
    restoreDirButton_->setEnabled(!isRunning);
    passwordEdit_->setEnabled(!isRunning);
    modePackageRadio_->setEnabled(!isRunning);
    modeFolderRadio_->setEnabled(!isRunning);
}

} // namespace DataBackup