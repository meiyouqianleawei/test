#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QTimer>
#include <memory>
#include "SchedulerWorker.h"
#include "FilterDialog.h"

namespace DataBackup {

/**
 * 定时任务选项卡
 * 提供用户界面管理定时备份任务
 */
class SchedulerTab : public QWidget {
    Q_OBJECT

public:
    /**
     * 构造函数
     * @param parent 父窗口
     */
    explicit SchedulerTab(QWidget* parent = nullptr);

    /**
     * 析构函数
     */
    ~SchedulerTab() override;

private slots:
    /**
     * 添加任务
     */
    void addTask();

    /**
     * 删除任务
     */
    void removeTask();

    /**
     * 暂停任务
     */
    void pauseTask();

    /**
     * 恢复任务
     */
    void resumeTask();

    /**
     * 手动执行任务
     */
    void executeTask();

    /**
     * 启动调度器
     */
    void startScheduler();

    /**
     * 停止调度器
     */
    void stopScheduler();

    /**
     * 刷新任务列表
     */
    void refreshTaskList();

    /**
     * 任务执行完成
     * @param result 执行结果
     */
    void onTaskExecuted(const TaskExecutionResult& result);

    /**
     * 任务类型改变
     * @param index 类型索引
     */
    void onTaskTypeChanged(int index);

    /**
     * 表格选择改变
     */
    void onSelectionChanged();

    /**
     * 加密选项改变
     */
    void onEncryptionChanged(bool checked);

    /**
     * 打开过滤器配置对话框
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
     */
    void updateButtonState();

    /**
     * 获取选中的任务ID
     * @return 任务ID，如果未选中返回空字符串
     */
    QString getSelectedTaskId() const;

    /**
     * 获取选中任务的状态
     */
    TaskStatus getSelectedTaskStatus() const;

    /**
     * 格式化时间戳
     * @param timestamp 时间戳（毫秒）
     * @return 格式化的时间字符串
     */
    QString formatTimestamp(uint64_t timestamp) const;

private:
    // 任务列表
    QTableWidget* taskTable_;

    // 任务配置输入
    QLineEdit* taskNameEdit_;
    QComboBox* taskTypeCombo_;

    // 时间相关（每天/每周/每月/一次性使用）
    QLabel* timeLabel_;
    QSpinBox* hourSpin_;
    QSpinBox* minuteSpin_;
    QLabel* hourUnitLabel_;
    QLabel* minuteUnitLabel_;

    // 周任务：星期几
    QLabel* dayOfWeekLabel_;
    QComboBox* dayOfWeekCombo_;

    // 月任务：日期
    QLabel* dayOfMonthLabel_;
    QSpinBox* dayOfMonthSpin_;

    // 间隔任务
    QLabel* intervalLabel_;
    QSpinBox* intervalHourSpin_;
    QSpinBox* intervalMinuteSpin_;
    QSpinBox* intervalSecondSpin_;
    QLabel* intervalHourUnitLabel_;
    QLabel* intervalMinuteUnitLabel_;
    QLabel* intervalSecondUnitLabel_;

    // 路径
    QLineEdit* sourcePathEdit_;
    QLineEdit* targetPathEdit_;
    QPushButton* browseSourceButton_;
    QPushButton* browseTargetButton_;

    // 压缩
    QComboBox* compressionCombo_;

    // 加密
    QCheckBox* encryptionCheck_;
    QLineEdit* passwordEdit_;

    // 过滤器
    QPushButton* filterButton_;
    QLabel* filterStatusLabel_;
    FilterOptions filterOptions_;

    // 控制按钮
    QPushButton* addTaskButton_;
    QPushButton* removeTaskButton_;
    QPushButton* pauseTaskButton_;
    QPushButton* resumeTaskButton_;
    QPushButton* executeButton_;
    QPushButton* refreshButton_;
    QPushButton* startSchedulerButton_;
    QPushButton* stopSchedulerButton_;

    // 状态显示
    QLabel* statusLabel_;

    // 定时刷新列表
    QTimer* refreshTimer_;

    // 调度器工作类
    SchedulerWorker* schedulerWorker_;
};

} // namespace DataBackup
