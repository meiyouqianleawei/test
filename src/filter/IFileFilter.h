#ifndef IFILEFILTER_H
#define IFILEFILTER_H

#include <string>
#include <memory>
#include "../core/FileInfo.h"

/**
 * @brief 文件过滤器接口（策略模式）
 *
 * 所有具体过滤器都必须实现此接口。
 * 使用策略模式，可以灵活组合不同的过滤规则。
 */
class IFileFilter {
public:
    virtual ~IFileFilter() = default;

    /**
     * @brief 判断文件是否应该包含在备份中
     * @param fileInfo 文件信息
     * @return bool 返回true表示包含，返回false表示排除
     */
    virtual bool accept(const FileInfo& fileInfo) const = 0;

    /**
     * @brief 获取过滤器名称（用于日志和调试）
     * @return std::string 过滤器名称
     */
    virtual std::string getName() const = 0;

    /**
     * @brief 获取过滤器描述（用于GUI显示）
     * @return std::string 过滤器描述
     */
    virtual std::string getDescription() const = 0;
};

#endif // IFILEFILTER_H