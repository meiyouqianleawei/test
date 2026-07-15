#include "SchedulerWorker.h"
#include <random>
#include <sstream>
#include <iomanip>

namespace DataBackup {

namespace {
    std::string makeTaskId() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 999999);
        std::stringstream ss;
        ss << "TASK_" << std::setw(6) << std::setfill('0') << dis(gen);
        return ss.str();
    }
}

SchedulerWorker::SchedulerWorker(QObject* parent)
    : QObject(parent)
    , scheduler_(std::make_unique<BackupScheduler>())
{
}

SchedulerWorker::~SchedulerWorker() {
    stop();
}

void SchedulerWorker::start() {
    if (scheduler_) {
        scheduler_->start();
    }
}

void SchedulerWorker::stop() {
    if (scheduler_) {
        scheduler_->stop();
    }
}

bool SchedulerWorker::addTask(const ScheduleConfig& config) {
    if (!scheduler_) {
        return false;
    }

    // 保证 taskId 非空，方便设置回调
    ScheduleConfig cfg = config;
    if (cfg.taskId.empty()) {
        cfg.taskId = makeTaskId();
    }

    bool result = scheduler_->addTask(cfg);

    // 设置任务回调
    if (result) {
        scheduler_->setTaskCallback(cfg.taskId, [this](const TaskExecutionResult& r) {
            emit taskExecuted(r);
        });
    }

    return result;
}

bool SchedulerWorker::removeTask(const std::string& taskId) {
    if (!scheduler_) {
        return false;
    }

    return scheduler_->removeTask(taskId);
}

bool SchedulerWorker::pauseTask(const std::string& taskId) {
    if (!scheduler_) {
        return false;
    }

    return scheduler_->pauseTask(taskId);
}

bool SchedulerWorker::resumeTask(const std::string& taskId) {
    if (!scheduler_) {
        return false;
    }

    return scheduler_->resumeTask(taskId);
}

void SchedulerWorker::executeTask(const std::string& taskId) {
    if (!scheduler_) {
        return;
    }

    TaskExecutionResult result = scheduler_->executeTask(taskId);
    emit taskExecuted(result);
}

std::vector<TaskInfo> SchedulerWorker::getTaskList() const {
    if (!scheduler_) {
        return std::vector<TaskInfo>();
    }

    return scheduler_->getTaskList();
}

bool SchedulerWorker::isRunning() const {
    if (!scheduler_) {
        return false;
    }

    return scheduler_->isRunning();
}

} // namespace DataBackup