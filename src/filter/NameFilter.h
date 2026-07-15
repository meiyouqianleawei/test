#ifndef NAMEFILTER_H
#define NAMEFILTER_H

#include "IFileFilter.h"
#include <string>

/**
 * @brief 文件名过滤器
 *
 * 根据文件名过滤，支持：
 * - 通配符匹配（* 和 ?）
 * - 正则表达式匹配（可选）
 * - 不区分大小写匹配
 */
class NameFilter : public IFileFilter {
public:
    /**
     * @brief 构造文件名过滤器
     * @param pattern 匹配模式（例如："*.txt", "image?.jpg"）
     * @param useRegex 是否使用正则表达式（默认false，使用通配符）
     */
    explicit NameFilter(const std::string& pattern, bool useRegex = false);

    /**
     * @brief 设置匹配模式
     * @param pattern 匹配模式
     */
    void setPattern(const std::string& pattern);

    // 实现IFileFilter接口
    bool accept(const FileInfo& fileInfo) const override;
    std::string getName() const override;
    std::string getDescription() const override;

private:
    std::string pattern_;
    bool useRegex_;

    /**
     * @brief 通配符匹配
     * @param text 要匹配的文本
     * @param pattern 通配符模式
     * @return bool 匹配返回true
     */
    bool wildcardMatch(const std::string& text, const std::string& pattern) const;

    /**
     * @brief 提取文件名（不含路径）
     * @param filePath 文件路径
     * @return std::string 文件名
     */
    std::string extractFileName(const std::string& filePath) const;
};

#endif // NAMEFILTER_H