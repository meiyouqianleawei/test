#include "TimeFilter.h"

TimeFilter::TimeFilter(TimeType timeType, uint64_t startTime, uint64_t endTime)
    : timeType_(timeType), startTime_(startTime), endTime_(endTime) {
}

void TimeFilter::setTimeRange(uint64_t startTime, uint64_t endTime) {
    startTime_ = startTime;
    endTime_ = endTime;
}

bool TimeFilter::accept(const FileInfo& fileInfo) const {
    uint64_t fileTime = getTime(fileInfo);

    // 0表示不限制
    if (startTime_ == 0 && endTime_ == 0) {
        return true;
    }

    // 检查时间范围
    bool afterStart = (startTime_ == 0) || (fileTime >= startTime_);
    bool beforeEnd = (endTime_ == 0) || (fileTime <= endTime_);

    return afterStart && beforeEnd;
}

uint64_t TimeFilter::getTime(const FileInfo& fileInfo) const {
    switch (timeType_) {
        case TimeType::CREATION:
            return fileInfo.creationTime;
        case TimeType::LAST_WRITE:
            return fileInfo.lastWriteTime;
        case TimeType::LAST_ACCESS:
            return fileInfo.lastAccessTime;
        default:
            return 0;
    }
}

std::string TimeFilter::getName() const {
    return "TimeFilter";
}

std::string TimeFilter::getDescription() const {
    std::string desc = timeTypeToString(timeType_) + "时间过滤器";

    if (startTime_ > 0 || endTime_ > 0) {
        desc += " [范围: ";
        if (startTime_ > 0) desc += "起始时间已设置";
        else desc += "无起始限制";

        desc += " - ";

        if (endTime_ > 0) desc += "结束时间已设置";
        else desc += "无结束限制";

        desc += "]";
    }

    return desc;
}

std::string TimeFilter::timeTypeToString(TimeType timeType) const {
    switch (timeType) {
        case TimeType::CREATION:
            return "创建";
        case TimeType::LAST_WRITE:
            return "修改";
        case TimeType::LAST_ACCESS:
            return "访问";
        default:
            return "未知";
    }
}