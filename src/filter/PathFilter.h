#ifndef PATHFILTER_H
#define PATHFILTER_H

#include "IFileFilter.h"
#include <vector>
#include <string>

/**
 * @brief 路径过滤器
 *
 * 根据文件路径过滤，支持：
 * - 包含特定路径前缀的文件
 * - 排除特定路径前缀的文件
 * - 支持多个路径规则
 */
class PathFilter : public IFileFilter {
public:
    /**
     * @brief 构造路径过滤器
     * @param includePaths 包含的路径前缀列表（空表示接受所有）
     * @param excludePaths 排除的路径前缀列表
     */
    PathFilter(const std::vector<std::string>& includePaths = {},
               const std::vector<std::string>& excludePaths = {});

    /**
     * @brief 添加包含路径
     * @param path 路径前缀
     */
    void addIncludePath(const std::string& path);

    /**
     * @brief 添加排除路径
     * @param path 路径前缀
     */
    void addExcludePath(const std::string& path);

    /**
     * @brief 清空包含路径列表
     */
    void clearIncludePaths();

    /**
     * @brief 清空排除路径列表
     */
    void clearExcludePaths();

    // 实现IFileFilter接口
    bool accept(const FileInfo& fileInfo) const override;
    std::string getName() const override;
    std::string getDescription() const override;

private:
    std::vector<std::string> includePaths_;
    std::vector<std::string> excludePaths_;

    /**
     * @brief 检查路径是否匹配前缀（不区分大小写）
     * @param path 要检查的路径
     * @param prefix 前缀
     * @return bool 匹配返回true
     */
    bool matchesPrefix(const std::string& path, const std::string& prefix) const;
};

#endif // PATHFILTER_H