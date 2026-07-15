#include "SchedulerTab.h"
#include <QFileDialog>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QMessageBox>
#include <QHeaderView>
#include <QDateTime>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>

namespace DataBackup {

SchedulerTab::SchedulerTab(QWidget* parent)
    : QWidget(parent)
    , taskTable_(nullptr)
    , taskNameEdit_(nullptr)
    , taskTypeCombo_(nullptr)
    , timeLabel_(nullptr)
    , hourSpin_(nullptr)
    , minuteSpin_(nullptr)
    , hourUnitLabel_(nullptr)
    , minuteUnitLabel_(nullptr)
    , dayOfWeekLabel_(nullptr)
    , dayOfWeekCombo_(nullptr)
    , dayOfMonthLabel_(nullptr)
    , dayOfMonthSpin_(nullptr)
    , intervalLabel_(nullptr)
    , intervalHourSpin_(nullptr)
    , intervalMinuteSpin_(nullptr)
    , intervalSecondSpin_(nullptr)
    , intervalHourUnitLabel_(nullptr)
    , intervalMinuteUnitLabel_(nullptr)
    , intervalSecondUnitLabel_(nullptr)
    , sourcePathEdit_(nullptr)
    , targetPathEdit_(nullptr)
    , browseSourceButton_(nullptr)
    , browseTargetButton_(nullptr)
    , compressionCombo_(nullptr)
    , encryptionCheck_(nullptr)
    , passwordEdit_(nullptr)
    , filterButton_(nullptr)
    , filterStatusLabel_(nullptr)
    , addTaskButton_(nullptr)
    , removeTaskButton_(nullptr)
    , pauseTaskButton_(nullptr)
    , resumeTaskButton_(nullptr)
    , executeButton_(nullptr)
    , refreshButton_(nullptr)
    , startSchedulerButton_(nullptr)
    , stopSchedulerButton_(nullptr)
    , statusLabel_(nullptr)
    , refreshTimer_(nullptr)
    , schedulerWorker_(nullptr)
{
    setupUI();

    // 初始化调度器工作线程（在 createConnections 之前，以便信号可以绑定）
    schedulerWorker_ = new SchedulerWorker(this);
    connect(schedulerWorker_, &SchedulerWorker::taskExecuted,
            this, &SchedulerTab::onTaskExecuted, Qt::QueuedConnection);

    createConnections();

    // 触发一次任务类型显示切换（默认选中 ONCE）
    onTaskTypeChanged(taskTypeCombo_->currentIndex());

    // 定时刷新任务列表（更新下次执行时间显示）
    refreshTimer_ = new QTimer(this);
    connect(refreshTimer_, &QTimer::timeout, this, &SchedulerTab::refreshTaskList);
    refreshTimer_->start(2000);

    refreshTaskList();
    updateButtonState();
}

SchedulerTab::~SchedulerTab() {
    if (refreshTimer_) {
        refreshTimer_->stop();
    }
    if (schedulerWorker_) {
        schedulerWorker_->stop();
    }
}

void SchedulerTab::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // 任务表格
    taskTable_ = new QTableWidget(this);
    taskTable_->setColumnCount(5);
    taskTable_->setHorizontalHeaderLabels({"任务名称", "任务类型", "状态", "下次执行时间", "任务ID"});
    taskTable_->horizontalHeader()->setStretchLastSection(false);
    taskTable_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    taskTable_->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    taskTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    taskTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    taskTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    taskTable_->hideColumn(4); // 隐藏任务ID列

    mainLayout->addWidget(taskTable_);

    // 任务配置区域
    QGroupBox* configGroup = new QGroupBox("任务配置", this);
    QGridLayout* configLayout = new QGridLayout(configGroup);

    // 任务名称
    taskNameEdit_ = new QLineEdit(this);
    taskNameEdit_->setPlaceholderText("输入任务名称");

    // 任务类型
    taskTypeCombo_ = new QComboBox(this);
    taskTypeCombo_->addItem("一次性任务", static_cast<int>(ScheduleType::ONCE));
    taskTypeCombo_->addItem("每天执行", static_cast<int>(ScheduleType::DAILY));
    taskTypeCombo_->addItem("每周执行", static_cast<int>(ScheduleType::WEEKLY));
    taskTypeCombo_->addItem("每月执行", static_cast<int>(ScheduleType::MONTHLY));
    taskTypeCombo_->addItem("间隔执行", static_cast<int>(ScheduleType::INTERVAL));

    // 执行时间（时:分）
    timeLabel_ = new QLabel("执行时间:", this);
    hourSpin_ = new QSpinBox(this);
    hourSpin_->setRange(0, 23);
    hourSpin_->setValue(QDateTime::currentDateTime().time().hour());
    minuteSpin_ = new QSpinBox(this);
    minuteSpin_->setRange(0, 59);
    minuteSpin_->setValue(0);
    hourUnitLabel_ = new QLabel("时", this);
    minuteUnitLabel_ = new QLabel("分", this);

    // 星期几（周任务）
    dayOfWeekLabel_ = new QLabel("星期:", this);
    dayOfWeekCombo_ = new QComboBox(this);
    dayOfWeekCombo_->addItem("星期日", static_cast<int>(DayOfWeek::SUNDAY));
    dayOfWeekCombo_->addItem("星期一", static_cast<int>(DayOfWeek::MONDAY));
    dayOfWeekCombo_->addItem("星期二", static_cast<int>(DayOfWeek::TUESDAY));
    dayOfWeekCombo_->addItem("星期三", static_cast<int>(DayOfWeek::WEDNESDAY));
    dayOfWeekCombo_->addItem("星期四", static_cast<int>(DayOfWeek::THURSDAY));
    dayOfWeekCombo_->addItem("星期五", static_cast<int>(DayOfWeek::FRIDAY));
    dayOfWeekCombo_->addItem("星期六", static_cast<int>(DayOfWeek::SATURDAY));

    // 每月第几天（月任务）
    dayOfMonthLabel_ = new QLabel("日期:", this);
    dayOfMonthSpin_ = new QSpinBox(this);
    dayOfMonthSpin_->setRange(1, 31);
    dayOfMonthSpin_->setValue(1);
    dayOfMonthSpin_->setSuffix(" 日");

    // 间隔（时/分/秒）
    intervalLabel_ = new QLabel("间隔:", this);
    intervalHourSpin_ = new QSpinBox(this);
    intervalHourSpin_->setRange(0, 999);
    intervalHourSpin_->setValue(0);
    intervalMinuteSpin_ = new QSpinBox(this);
    intervalMinuteSpin_->setRange(0, 59);
    intervalMinuteSpin_->setValue(30);
    intervalSecondSpin_ = new QSpinBox(this);
    intervalSecondSpin_->setRange(0, 59);
    intervalSecondSpin_->setValue(0);
    intervalHourUnitLabel_ = new QLabel("时", this);
    intervalMinuteUnitLabel_ = new QLabel("分", this);
    intervalSecondUnitLabel_ = new QLabel("秒", this);

    // 路径
    sourcePathEdit_ = new QLineEdit(this);
    sourcePathEdit_->setPlaceholderText("源目录");
    browseSourceButton_ = new QPushButton("浏览...", this);

    targetPathEdit_ = new QLineEdit(this);
    targetPathEdit_->setPlaceholderText("目标文件（.pkg）");
    browseTargetButton_ = new QPushButton("浏览...", this);

    // 压缩
    compressionCombo_ = new QComboBox(this);
    compressionCombo_->addItem("无压缩", "none");
    compressionCombo_->addItem("Huffman压缩", "huffman");
    compressionCombo_->addItem("Zlib压缩", "zlib");

    // 加密
    encryptionCheck_ = new QCheckBox("启用加密", this);
    passwordEdit_ = new QLineEdit(this);
    passwordEdit_->setEchoMode(QLineEdit::Password);
    passwordEdit_->setPlaceholderText("密码");
    passwordEdit_->setEnabled(false);

    // 过滤器
    filterButton_ = new QPushButton("配置过滤器...", this);
    filterStatusLabel_ = new QLabel("未启用过滤", this);
    filterStatusLabel_->setStyleSheet("color: gray;");

    // 布局：使用两列的紧凑布局
    int row = 0;
    configLayout->addWidget(new QLabel("任务名称:", this), row, 0);
    configLayout->addWidget(taskNameEdit_, row, 1, 1, 4);
    row++;

    configLayout->addWidget(new QLabel("任务类型:", this), row, 0);
    configLayout->addWidget(taskTypeCombo_, row, 1, 1, 4);
    row++;

    // 执行时间行（时:分）
    configLayout->addWidget(timeLabel_, row, 0);
    {
        QHBoxLayout* timeLayout = new QHBoxLayout();
        timeLayout->addWidget(hourSpin_);
        timeLayout->addWidget(hourUnitLabel_);
        timeLayout->addSpacing(8);
        timeLayout->addWidget(minuteSpin_);
        timeLayout->addWidget(minuteUnitLabel_);
        timeLayout->addStretch();
        configLayout->addLayout(timeLayout, row, 1, 1, 4);
    }
    row++;

    // 星期
    configLayout->addWidget(dayOfWeekLabel_, row, 0);
    configLayout->addWidget(dayOfWeekCombo_, row, 1, 1, 4);
    row++;

    // 日期
    configLayout->addWidget(dayOfMonthLabel_, row, 0);
    configLayout->addWidget(dayOfMonthSpin_, row, 1, 1, 4);
    row++;

    // 间隔
    configLayout->addWidget(intervalLabel_, row, 0);
    {
        QHBoxLayout* intervalLayout = new QHBoxLayout();
        intervalLayout->addWidget(intervalHourSpin_);
        intervalLayout->addWidget(intervalHourUnitLabel_);
        intervalLayout->addSpacing(8);
        intervalLayout->addWidget(intervalMinuteSpin_);
        intervalLayout->addWidget(intervalMinuteUnitLabel_);
        intervalLayout->addSpacing(8);
        intervalLayout->addWidget(intervalSecondSpin_);
        intervalLayout->addWidget(intervalSecondUnitLabel_);
        intervalLayout->addStretch();
        configLayout->addLayout(intervalLayout, row, 1, 1, 4);
    }
    row++;

    // 源目录
    configLayout->addWidget(new QLabel("源目录:", this), row, 0);
    configLayout->addWidget(sourcePathEdit_, row, 1, 1, 3);
    configLayout->addWidget(browseSourceButton_, row, 4);
    row++;

    // 目标路径
    configLayout->addWidget(new QLabel("目标路径:", this), row, 0);
    configLayout->addWidget(targetPathEdit_, row, 1, 1, 3);
    configLayout->addWidget(browseTargetButton_, row, 4);
    row++;

    // 压缩
    configLayout->addWidget(new QLabel("压缩方式:", this), row, 0);
    configLayout->addWidget(compressionCombo_, row, 1, 1, 4);
    row++;

    // 加密
    configLayout->addWidget(encryptionCheck_, row, 0);
    configLayout->addWidget(passwordEdit_, row, 1, 1, 4);
    row++;

    // 过滤器
    configLayout->addWidget(new QLabel("文件过滤:", this), row, 0);
    configLayout->addWidget(filterStatusLabel_, row, 1, 1, 3);
    configLayout->addWidget(filterButton_, row, 4);
    row++;

    mainLayout->addWidget(configGroup);

    // 控制按钮
    QHBoxLayout* buttonLayout = new QHBoxLayout();

    addTaskButton_ = new QPushButton("添加任务", this);
    removeTaskButton_ = new QPushButton("删除任务", this);
    pauseTaskButton_ = new QPushButton("暂停", this);
    resumeTaskButton_ = new QPushButton("恢复", this);
    executeButton_ = new QPushButton("立即执行", this);
    refreshButton_ = new QPushButton("刷新", this);
    startSchedulerButton_ = new QPushButton("启动调度器", this);
    stopSchedulerButton_ = new QPushButton("停止调度器", this);

    buttonLayout->addWidget(addTaskButton_);
    buttonLayout->addWidget(removeTaskButton_);
    buttonLayout->addWidget(pauseTaskButton_);
    buttonLayout->addWidget(resumeTaskButton_);
    buttonLayout->addWidget(executeButton_);
    buttonLayout->addWidget(refreshButton_);
    buttonLayout->addStretch();
    buttonLayout->addWidget(startSchedulerButton_);
    buttonLayout->addWidget(stopSchedulerButton_);

    mainLayout->addLayout(buttonLayout);

    // 状态显示
    statusLabel_ = new QLabel("调度器未启动", this);
    mainLayout->addWidget(statusLabel_);
}

void SchedulerTab::createConnections() {
    connect(addTaskButton_, &QPushButton::clicked, this, &SchedulerTab::addTask);
    connect(removeTaskButton_, &QPushButton::clicked, this, &SchedulerTab::removeTask);
    connect(pauseTaskButton_, &QPushButton::clicked, this, &SchedulerTab::pauseTask);
    connect(resumeTaskButton_, &QPushButton::clicked, this, &SchedulerTab::resumeTask);
    connect(executeButton_, &QPushButton::clicked, this, &SchedulerTab::executeTask);
    connect(refreshButton_, &QPushButton::clicked, this, &SchedulerTab::refreshTaskList);
    connect(startSchedulerButton_, &QPushButton::clicked, this, &SchedulerTab::startScheduler);
    connect(stopSchedulerButton_, &QPushButton::clicked, this, &SchedulerTab::stopScheduler);

    connect(browseSourceButton_, &QPushButton::clicked, [this]() {
        QString dir = QFileDialog::getExistingDirectory(this, "选择源目录",
            sourcePathEdit_->text(), QFileDialog::ShowDirsOnly);
        if (!dir.isEmpty()) {
            sourcePathEdit_->setText(dir);
        }
    });

    connect(browseTargetButton_, &QPushButton::clicked, [this]() {
        QString file = QFileDialog::getSaveFileName(this, "选择备份文件",
            targetPathEdit_->text().isEmpty() ? "backup.pkg" : targetPathEdit_->text(),
            "备份文件 (*.pkg);;所有文件 (*)");
        if (!file.isEmpty()) {
            if (!file.endsWith(".pkg", Qt::CaseInsensitive) &&
                !file.endsWith(".backup", Qt::CaseInsensitive)) {
                file += ".pkg";
            }
            targetPathEdit_->setText(file);
        }
    });

    connect(taskTypeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SchedulerTab::onTaskTypeChanged);

    connect(encryptionCheck_, &QCheckBox::toggled, this, &SchedulerTab::onEncryptionChanged);
    connect(filterButton_, &QPushButton::clicked, this, &SchedulerTab::openFilterDialog);

    connect(taskTable_, &QTableWidget::itemSelectionChanged,
            this, &SchedulerTab::onSelectionChanged);
}

void SchedulerTab::addTask() {
    if (taskNameEdit_->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "警告", "请输入任务名称");
        return;
    }

    if (sourcePathEdit_->text().isEmpty() || targetPathEdit_->text().isEmpty()) {
        QMessageBox::warning(this, "警告", "请选择源目录和目标路径");
        return;
    }

    ScheduleType type = static_cast<ScheduleType>(taskTypeCombo_->currentData().toInt());

    // 间隔任务必须有非零间隔
    if (type == ScheduleType::INTERVAL) {
        int totalSec = intervalHourSpin_->value() * 3600
                     + intervalMinuteSpin_->value() * 60
                     + intervalSecondSpin_->value();
        if (totalSec <= 0) {
            QMessageBox::warning(this, "警告", "间隔时间必须大于 0");
            return;
        }
    }

    // 加密必须有密码
    if (encryptionCheck_->isChecked() && passwordEdit_->text().isEmpty()) {
        QMessageBox::warning(this, "警告", "启用加密时必须输入密码");
        return;
    }

    ScheduleConfig config;
    config.taskName = taskNameEdit_->text().trimmed().toStdString();
    config.type = type;
    config.hour = hourSpin_->value();
    config.minute = minuteSpin_->value();
    config.second = 0;
    config.dayOfWeek = static_cast<DayOfWeek>(dayOfWeekCombo_->currentData().toInt());
    config.dayOfMonth = dayOfMonthSpin_->value();
    config.intervalHours = intervalHourSpin_->value();
    config.intervalMinutes = intervalMinuteSpin_->value();
    config.intervalSeconds = intervalSecondSpin_->value();
    config.enabled = true;

    // 备份配置
    config.backupConfig.sourcePath = sourcePathEdit_->text().toStdString();
    config.backupConfig.targetPath = targetPathEdit_->text().toStdString();
    config.backupConfig.compressionAlgorithm =
        compressionCombo_->currentData().toString().toStdString();
    if (encryptionCheck_->isChecked()) {
        config.backupConfig.encryptionAlgorithm = "xor";
        config.backupConfig.password = passwordEdit_->text().toStdString();
    } else {
        config.backupConfig.encryptionAlgorithm = "none";
    }
    if (filterOptions_.anyEnabled()) {
        config.backupConfig.filters = filterOptions_.buildFilters();
        config.backupConfig.filterModeAnd = (filterOptions_.mode == FilterChain::Mode::AND);
    }

    if (schedulerWorker_->addTask(config)) {
        QMessageBox::information(this, "成功", "任务添加成功");
        refreshTaskList();

        // 清空输入
        taskNameEdit_->clear();
        sourcePathEdit_->clear();
        targetPathEdit_->clear();
        passwordEdit_->clear();
        encryptionCheck_->setChecked(false);
    } else {
        QMessageBox::warning(this, "错误", "任务添加失败（任务名或ID可能重复）");
    }
}

void SchedulerTab::removeTask() {
    QString taskId = getSelectedTaskId();
    if (taskId.isEmpty()) {
        QMessageBox::warning(this, "警告", "请选择要删除的任务");
        return;
    }

    if (QMessageBox::question(this, "确认", "确定要删除选中的任务吗？") == QMessageBox::Yes) {
        if (schedulerWorker_->removeTask(taskId.toStdString())) {
            statusLabel_->setText("任务已删除");
            refreshTaskList();
        } else {
            QMessageBox::warning(this, "错误", "删除失败");
        }
    }
}

void SchedulerTab::pauseTask() {
    QString taskId = getSelectedTaskId();
    if (taskId.isEmpty()) {
        QMessageBox::warning(this, "警告", "请选择要暂停的任务");
        return;
    }

    if (schedulerWorker_->pauseTask(taskId.toStdString())) {
        statusLabel_->setText("任务已暂停");
        refreshTaskList();
    } else {
        QMessageBox::warning(this, "错误", "暂停失败");
    }
}

void SchedulerTab::resumeTask() {
    QString taskId = getSelectedTaskId();
    if (taskId.isEmpty()) {
        QMessageBox::warning(this, "警告", "请选择要恢复的任务");
        return;
    }

    if (schedulerWorker_->resumeTask(taskId.toStdString())) {
        statusLabel_->setText("任务已恢复");
        refreshTaskList();
    } else {
        QMessageBox::warning(this, "错误", "恢复失败");
    }
}

void SchedulerTab::executeTask() {
    QString taskId = getSelectedTaskId();
    if (taskId.isEmpty()) {
        QMessageBox::warning(this, "警告", "请选择要执行的任务");
        return;
    }

    statusLabel_->setText("正在执行任务...");
    schedulerWorker_->executeTask(taskId.toStdString());
}

void SchedulerTab::startScheduler() {
    schedulerWorker_->start();
    statusLabel_->setText("调度器已启动");
    updateButtonState();
    refreshTaskList();
}

void SchedulerTab::stopScheduler() {
    schedulerWorker_->stop();
    statusLabel_->setText("调度器已停止");
    updateButtonState();
    refreshTaskList();
}

void SchedulerTab::refreshTaskList() {
    // 保存当前选中的任务ID，刷新后尝试恢复选中
    QString selectedId = getSelectedTaskId();

    std::vector<TaskInfo> tasks = schedulerWorker_->getTaskList();

    taskTable_->setRowCount(static_cast<int>(tasks.size()));

    int restoreRow = -1;
    for (size_t i = 0; i < tasks.size(); ++i) {
        const TaskInfo& task = tasks[i];

        taskTable_->setItem(static_cast<int>(i), 0,
            new QTableWidgetItem(QString::fromStdString(task.config.taskName)));

        QString typeStr;
        switch (task.config.type) {
            case ScheduleType::ONCE: typeStr = "一次性"; break;
            case ScheduleType::DAILY: typeStr = "每天"; break;
            case ScheduleType::WEEKLY: typeStr = "每周"; break;
            case ScheduleType::MONTHLY: typeStr = "每月"; break;
            case ScheduleType::INTERVAL: typeStr = "间隔"; break;
        }
        taskTable_->setItem(static_cast<int>(i), 1, new QTableWidgetItem(typeStr));

        QString statusStr;
        switch (task.status) {
            case TaskStatus::IDLE: statusStr = "空闲"; break;
            case TaskStatus::RUNNING: statusStr = "运行中"; break;
            case TaskStatus::PAUSED: statusStr = "已暂停"; break;
            case TaskStatus::COMPLETED: statusStr = "已完成"; break;
            case TaskStatus::FAILED: statusStr = "失败"; break;
        }
        taskTable_->setItem(static_cast<int>(i), 2, new QTableWidgetItem(statusStr));

        taskTable_->setItem(static_cast<int>(i), 3,
            new QTableWidgetItem(formatTimestamp(task.nextExecutionTime)));

        QString idStr = QString::fromStdString(task.config.taskId);
        taskTable_->setItem(static_cast<int>(i), 4, new QTableWidgetItem(idStr));

        if (!selectedId.isEmpty() && idStr == selectedId) {
            restoreRow = static_cast<int>(i);
        }
    }

    if (restoreRow >= 0) {
        taskTable_->selectRow(restoreRow);
    }

    updateButtonState();
}

void SchedulerTab::onTaskExecuted(const TaskExecutionResult& result) {
    refreshTaskList();

    if (result.success) {
        statusLabel_->setText(QString("任务 '%1' 执行成功（%2 个文件，%3 字节）")
            .arg(QString::fromStdString(result.taskName))
            .arg(result.filesBackedup)
            .arg(result.backupSize));
    } else {
        statusLabel_->setText(QString("任务 '%1' 执行失败: %2")
            .arg(QString::fromStdString(result.taskName))
            .arg(QString::fromStdString(result.errorMessage)));
    }
}

void SchedulerTab::onTaskTypeChanged(int /*index*/) {
    ScheduleType type = static_cast<ScheduleType>(taskTypeCombo_->currentData().toInt());

    // 默认全部隐藏
    bool showTime = false;      // 时:分
    bool showDayOfWeek = false;
    bool showDayOfMonth = false;
    bool showInterval = false;

    switch (type) {
        case ScheduleType::ONCE:
        case ScheduleType::DAILY:
            showTime = true;
            break;
        case ScheduleType::WEEKLY:
            showTime = true;
            showDayOfWeek = true;
            break;
        case ScheduleType::MONTHLY:
            showTime = true;
            showDayOfMonth = true;
            break;
        case ScheduleType::INTERVAL:
            showInterval = true;
            break;
    }

    timeLabel_->setVisible(showTime);
    hourSpin_->setVisible(showTime);
    minuteSpin_->setVisible(showTime);
    hourUnitLabel_->setVisible(showTime);
    minuteUnitLabel_->setVisible(showTime);

    dayOfWeekLabel_->setVisible(showDayOfWeek);
    dayOfWeekCombo_->setVisible(showDayOfWeek);

    dayOfMonthLabel_->setVisible(showDayOfMonth);
    dayOfMonthSpin_->setVisible(showDayOfMonth);

    intervalLabel_->setVisible(showInterval);
    intervalHourSpin_->setVisible(showInterval);
    intervalMinuteSpin_->setVisible(showInterval);
    intervalSecondSpin_->setVisible(showInterval);
    intervalHourUnitLabel_->setVisible(showInterval);
    intervalMinuteUnitLabel_->setVisible(showInterval);
    intervalSecondUnitLabel_->setVisible(showInterval);
}

void SchedulerTab::onSelectionChanged() {
    updateButtonState();
}

void SchedulerTab::onEncryptionChanged(bool checked) {
    passwordEdit_->setEnabled(checked);
    if (!checked) {
        passwordEdit_->clear();
    }
}

void SchedulerTab::openFilterDialog() {
    FilterDialog dlg(filterOptions_, this);
    if (dlg.exec() == QDialog::Accepted) {
        filterOptions_ = dlg.options();
        filterStatusLabel_->setText(QString::fromStdString(filterOptions_.summary()));
        filterStatusLabel_->setStyleSheet(filterOptions_.anyEnabled()
                                          ? "color: #1e7e34;"
                                          : "color: gray;");
    }
}

void SchedulerTab::updateButtonState() {
    bool isRunning = schedulerWorker_ && schedulerWorker_->isRunning();
    bool hasSelection = !getSelectedTaskId().isEmpty();
    TaskStatus selStatus = hasSelection ? getSelectedTaskStatus() : TaskStatus::IDLE;

    startSchedulerButton_->setEnabled(!isRunning);
    stopSchedulerButton_->setEnabled(isRunning);

    // 选中任务时启用相关操作
    removeTaskButton_->setEnabled(hasSelection);
    executeButton_->setEnabled(hasSelection);

    // 暂停：仅当选中且未暂停/未失败/未完成
    pauseTaskButton_->setEnabled(hasSelection
        && selStatus != TaskStatus::PAUSED
        && selStatus != TaskStatus::FAILED);

    // 恢复：仅当已暂停或失败
    resumeTaskButton_->setEnabled(hasSelection
        && (selStatus == TaskStatus::PAUSED || selStatus == TaskStatus::FAILED));
}

QString SchedulerTab::getSelectedTaskId() const {
    int row = taskTable_->currentRow();
    if (row < 0 || row >= taskTable_->rowCount()) {
        return QString();
    }

    QTableWidgetItem* item = taskTable_->item(row, 4);
    return item ? item->text() : QString();
}

TaskStatus SchedulerTab::getSelectedTaskStatus() const {
    int row = taskTable_->currentRow();
    if (row < 0 || row >= taskTable_->rowCount()) {
        return TaskStatus::IDLE;
    }

    QTableWidgetItem* item = taskTable_->item(row, 2);
    if (!item) return TaskStatus::IDLE;

    QString text = item->text();
    if (text == "运行中") return TaskStatus::RUNNING;
    if (text == "已暂停") return TaskStatus::PAUSED;
    if (text == "已完成") return TaskStatus::COMPLETED;
    if (text == "失败") return TaskStatus::FAILED;
    return TaskStatus::IDLE;
}

QString SchedulerTab::formatTimestamp(uint64_t timestamp) const {
    if (timestamp == 0) {
        return "-";
    }

    // 时间戳单位为毫秒（BackupScheduler 中使用毫秒）
    QDateTime dateTime = QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(timestamp));
    return dateTime.toString("yyyy-MM-dd hh:mm:ss");
}

} // namespace DataBackup
