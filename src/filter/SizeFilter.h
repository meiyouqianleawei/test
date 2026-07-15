#ifndef SIZEFILTER_H
#define SIZEFILTER_H

#include "IFileFilter.h"
#include <cstdint>

/**
 * @brief 文件大小过滤器
 *
 * 根据文件大小过滤，支持：
 * - 最小大小限制
 * - 最大大小限制
 * - 支持各种大小单位（B/KB/MB/GB）
 */
class SizeFilter : public IFileFilter {
public:
    /**
     * @brief 构造大小过滤器
     * @param minSize 最小大小（字节，0表示不限制）
     * @param maxSize 最大大小（字节，0表示不限制）
     */
    SizeFilter(uint64_t minSize = 0, uint64_t maxSize = 0);

    /**
     * @brief 设置大小范围
     * @param minSize 最小大小
     * @param maxSize 最大大小
     */
    void setSizeRange(uint64_t minSize, uint64_t maxSize);

    /**
     * @brief 从人类可读格式创建过滤器
     * @param minSize 最小大小字符串（例如："1MB", "500KB"）
     * @param maxSize 最大大小字符串
     * @return SizeFilter 过滤器对象
     */
    static SizeFilter fromReadable(const std::string& minSize = "0B",
                                    const std::string& maxSize = "0B");

    // 实现IFileFilter接口
    bool accept(const FileInfo& fileInfo) const override;
    std::string getName() const override;
    std::string getDescription() const override;

private:
    uint64_t minSize_;
    uint64_t maxSize_;

    /**
     * @brief 解析大小字符串为字节数
     * @param sizeStr 大小字符串（例如："1MB", "500KB"）
     * @return uint64_t 字节数
     */
    static uint64_t parseSize(const std::string& sizeStr);

    /**
     * @brief 格式化大小为可读字符串
     * @param bytes 字节数
     * @return std::string 可读字符串
     */
    static std::string formatSize(uint64_t bytes);
};

#endif // SIZEFILTER_H