#include "BackupTab.h"
#include <QFileDialog>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QMessageBox>
#include <QThread>

namespace DataBackup {

BackupTab::BackupTab(QWidget* parent)
    : QWidget(parent)
    , sourceLabel_(nullptr)
    , sourceEdit_(nullptr)
    , sourceButton_(nullptr)
    , targetLabel_(nullptr)
    , targetEdit_(nullptr)
    , targetButton_(nullptr)
    , compressionLabel_(nullptr)
    , compressionCombo_(nullptr)
    , encryptionCheck_(nullptr)
    , passwordLabel_(nullptr)
    , passwordEdit_(nullptr)
    , startButton_(nullptr)
    , cancelButton_(nullptr)
    , progressBar_(nullptr)
    , statusLabel_(nullptr)
    , backupWorker_(nullptr)
    , filterButton_(nullptr)
    , filterStatusLabel_(nullptr)
{
    setupUI();
    createConnections();
}

BackupTab::~BackupTab() {
    if (backupWorker_ && backupWorker_->isRunning()) {
        backupWorker_->cancel();
        backupWorker_->wait();
    }
}

void BackupTab::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // 源目录选择
    QGridLayout* gridLayout = new QGridLayout();

    sourceLabel_ = new QLabel("源目录:", this);
    sourceEdit_ = new QLineEdit(this);
    sourceButton_ = new QPushButton("浏览...", this);

    gridLayout->addWidget(sourceLabel_, 0, 0);
    gridLayout->addWidget(sourceEdit_, 0, 1);
    gridLayout->addWidget(sourceButton_, 0, 2);

    // 目标路径选择
    targetLabel_ = new QLabel("目标路径:", this);
    targetEdit_ = new QLineEdit(this);
    targetButton_ = new QPushButton("浏览...", this);

    gridLayout->addWidget(targetLabel_, 1, 0);
    gridLayout->addWidget(targetEdit_, 1, 1);
    gridLayout->addWidget(targetButton_, 1, 2);

    // 压缩选项
    compressionLabel_ = new QLabel("压缩方式:", this);
    compressionCombo_ = new QComboBox(this);
    compressionCombo_->addItem("无压缩", "none");
    compressionCombo_->addItem("Huffman压缩", "huffman");
    compressionCombo_->addItem("Zlib压缩", "zlib");

    gridLayout->addWidget(compressionLabel_, 2, 0);
    gridLayout->addWidget(compressionCombo_, 2, 1);

    // 加密选项
    encryptionCheck_ = new QCheckBox("启用加密", this);
    passwordLabel_ = new QLabel("密码:", this);
    passwordEdit_ = new QLineEdit(this);
    passwordEdit_->setEchoMode(QLineEdit::Password);
    passwordEdit_->setEnabled(false);

    gridLayout->addWidget(encryptionCheck_, 3, 0);
    gridLayout->addWidget(passwordLabel_, 3, 1);
    gridLayout->addWidget(passwordEdit_, 3, 2);

    // 文件过滤
    QLabel* filterLabel = new QLabel("文件过滤:", this);
    filterStatusLabel_ = new QLabel("未启用过滤", this);
    filterStatusLabel_->setStyleSheet("color: gray;");
    filterButton_ = new QPushButton("配置过滤器...", this);

    gridLayout->addWidget(filterLabel, 4, 0);
    gridLayout->addWidget(filterStatusLabel_, 4, 1);
    gridLayout->addWidget(filterButton_, 4, 2);

    mainLayout->addLayout(gridLayout);

    // 控制按钮
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    startButton_ = new QPushButton("开始备份", this);
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

void BackupTab::createConnections() {
    connect(sourceButton_, &QPushButton::clicked, this, &BackupTab::selectSourceDirectory);
    connect(targetButton_, &QPushButton::clicked, this, &BackupTab::selectTargetPath);
    connect(startButton_, &QPushButton::clicked, this, &BackupTab::startBackup);
    connect(cancelButton_, &QPushButton::clicked, this, &BackupTab::cancelBackup);
    connect(encryptionCheck_, &QCheckBox::toggled, this, &BackupTab::onEncryptionChanged);
    connect(filterButton_, &QPushButton::clicked, this, &BackupTab::openFilterDialog);
}

void BackupTab::openFilterDialog() {
    FilterDialog dlg(filterOptions_, this);
    if (dlg.exec() == QDialog::Accepted) {
        filterOptions_ = dlg.options();
        filterStatusLabel_->setText(QString::fromStdString(filterOptions_.summary()));
        filterStatusLabel_->setStyleSheet(filterOptions_.anyEnabled()
                                          ? "color: #1e7e34;"
                                          : "color: gray;");
    }
}

void BackupTab::selectSourceDirectory() {
    QString dir = QFileDialog::getExistingDirectory(this, "选择源目录",
        sourceEdit_->text(), QFileDialog::ShowDirsOnly);
    if (!dir.isEmpty()) {
        sourceEdit_->setText(dir);
    }
}

void BackupTab::selectTargetPath() {
    // 使用保存文件对话框，允许用户输入新的文件名
    QString file = QFileDialog::getSaveFileName(this, "选择备份文件",
        targetEdit_->text().isEmpty() ? "backup.pkg" : targetEdit_->text(),
        "备份文件 (*.pkg);;所有文件 (*)");

    if (!file.isEmpty()) {
        // 如果没有扩展名，自动添加 .pkg
        if (!file.endsWith(".pkg", Qt::CaseInsensitive) &&
            !file.endsWith(".backup", Qt::CaseInsensitive)) {
            file += ".pkg";
        }
        targetEdit_->setText(file);
    }
}

void BackupTab::startBackup() {
    // 验证输入
    if (sourceEdit_->text().isEmpty()) {
        QMessageBox::warning(this, "警告", "请选择源目录");
        return;
    }

    if (targetEdit_->text().isEmpty()) {
        QMessageBox::warning(this, "警告", "请选择目标路径");
        return;
    }

    if (encryptionCheck_->isChecked() && passwordEdit_->text().isEmpty()) {
        QMessageBox::warning(this, "警告", "请输入加密密码");
        return;
    }

    // 准备配置
    BackupConfig config;
    config.sourcePath = sourceEdit_->text().toStdString();
    config.targetPath = targetEdit_->text().toStdString();
    config.compressionAlgorithm = compressionCombo_->currentData().toString().toStdString();
    
    if (encryptionCheck_->isChecked()) {
        config.encryptionAlgorithm = "xor";
        config.password = passwordEdit_->text().toStdString();
    }

    // 应用文件过滤器
    if (filterOptions_.anyEnabled()) {
        config.filters = filterOptions_.buildFilters();
        config.filterModeAnd = (filterOptions_.mode == FilterChain::Mode::AND);
    }

    // 创建并启动工作线程
    if (!backupWorker_) {
        backupWorker_ = new BackupWorker(this);
        connect(backupWorker_, &BackupWorker::progressUpdate,
                this, &BackupTab::onProgressUpdate, Qt::QueuedConnection);
        connect(backupWorker_, &BackupWorker::backupComplete,
                this, &BackupTab::onBackupComplete, Qt::QueuedConnection);
    }

    backupWorker_->setConfig(config);
    backupWorker_->start();

    updateButtonState(true);
    statusLabel_->setText("正在备份...");
}

void BackupTab::cancelBackup() {
    if (backupWorker_ && backupWorker_->isRunning()) {
        backupWorker_->cancel();
        statusLabel_->setText("正在取消...");
    }
}

void BackupTab::onProgressUpdate(const BackupProgress& progress) {
    progressBar_->setValue(progress.percentage);
    statusLabel_->setText(QString::fromStdString(progress.getPhaseName()) + 
                         " - " + QString::fromStdString(progress.currentFile));
}

void BackupTab::onBackupComplete(const BackupResult& result) {
    updateButtonState(false);

    if (result.success) {
        progressBar_->setValue(100);
        statusLabel_->setText(QString("备份完成 - 已备份 %1 个文件，总大小 %2 字节")
            .arg(result.filesBackedup)
            .arg(result.totalSize));

        QMessageBox::information(this, "成功",
            QString("备份完成!\n\n"
                    "备份文件: %1\n"
                    "文件数量: %2\n"
                    "原始大小: %3 字节\n"
                    "备份大小: %4 字节\n"
                    "压缩率: %5%%")
            .arg(QString::fromStdString(result.backupFilePath))
            .arg(result.filesBackedup)
            .arg(result.totalSize)
            .arg(result.backupSize)
            .arg(result.getCompressionRatio(), 0, 'f', 2));
    } else {
        statusLabel_->setText("备份失败");
        QMessageBox::critical(this, "错误",
            QString("备份失败:\n%1").arg(QString::fromStdString(result.errorMessage)));
    }
}

void BackupTab::onEncryptionChanged(bool checked) {
    passwordEdit_->setEnabled(checked);
    if (!checked) {
        passwordEdit_->clear();
    }
}

void BackupTab::updateButtonState(bool isRunning) {
    startButton_->setEnabled(!isRunning);
    cancelButton_->setEnabled(isRunning);
    sourceEdit_->setEnabled(!isRunning);
    targetEdit_->setEnabled(!isRunning);
    sourceButton_->setEnabled(!isRunning);
    targetButton_->setEnabled(!isRunning);
    compressionCombo_->setEnabled(!isRunning);
    encryptionCheck_->setEnabled(!isRunning);
    passwordEdit_->setEnabled(!isRunning && encryptionCheck_->isChecked());
    if (filterButton_) filterButton_->setEnabled(!isRunning);
}

} // namespace DataBackup