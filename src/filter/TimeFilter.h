#ifndef TIMEFILTER_H
#define TIMEFILTER_H

#include "IFileFilter.h"
#include <cstdint>

/**
 * @brief 时间过滤器
 *
 * 根据文件时间戳过滤，支持：
 * - 创建时间范围
 * - 修改时间范围
 * - 访问时间范围
 */
class TimeFilter : public IFileFilter {
public:
    /**
     * @brief 时间类型枚举
     */
    enum class TimeType {
        CREATION,     // 创建时间
        LAST_WRITE,   // 修改时间
        LAST_ACCESS   // 访问时间
    };

    /**
     * @brief 构造时间过滤器
     * @param timeType 时间类型
     * @param startTime 起始时间（FILETIME格式，0表示不限制）
     * @param endTime 结束时间（FILETIME格式，0表示不限制）
     */
    TimeFilter(TimeType timeType,
               uint64_t startTime = 0,
               uint64_t endTime = 0);

    /**
     * @brief 设置时间范围
     * @param startTime 起始时间
     * @param endTime 结束时间
     */
    void setTimeRange(uint64_t startTime, uint64_t endTime);

    // 实现IFileFilter接口
    bool accept(const FileInfo& fileInfo) const override;
    std::string getName() const override;
    std::string getDescription() const override;

private:
    TimeType timeType_;
    uint64_t startTime_;
    uint64_t endTime_;

    /**
     * @brief 获取指定类型的时间戳
     * @param fileInfo 文件信息
     * @return uint64_t 时间戳
     */
    uint64_t getTime(const FileInfo& fileInfo) const;

    /**
     * @brief 时间类型转字符串
     * @param timeType 时间类型
     * @return std::string 类型名称
     */
    std::string timeTypeToString(TimeType timeType) const;
};

#endif // TIMEFILTER_H