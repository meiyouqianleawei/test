#ifndef TYPEFILTER_H
#define TYPEFILTER_H

#include "IFileFilter.h"
#include <vector>
#include <string>

/**
 * @brief 文件类型过滤器
 *
 * 根据文件扩展名过滤，支持：
 * - 包含特定类型的文件
 * - 排除特定类型的文件
 * - 不区分大小写匹配
 */
class TypeFilter : public IFileFilter {
public:
    /**
     * @brief 构造类型过滤器
     * @param includeTypes 包含的文件类型列表（例如：{"txt", "pdf", "doc"}）
     * @param excludeTypes 排除的文件类型列表
     */
    TypeFilter(const std::vector<std::string>& includeTypes = {},
               const std::vector<std::string>& excludeTypes = {});

    /**
     * @brief 添加包含类型
     * @param type 文件扩展名（不带点）
     */
    void addIncludeType(const std::string& type);

    /**
     * @brief 添加排除类型
     * @param type 文件扩展名（不带点）
     */
    void addExcludeType(const std::string& type);

    /**
     * @brief 清空包含类型列表
     */
    void clearIncludeTypes();

    /**
     * @brief 清空排除类型列表
     */
    void clearExcludeTypes();

    // 实现IFileFilter接口
    bool accept(const FileInfo& fileInfo) const override;
    std::string getName() const override;
    std::string getDescription() const override;

private:
    std::vector<std::string> includeTypes_;
    std::vector<std::string> excludeTypes_;

    /**
     * @brief 提取文件扩展名（不带点，小写）
     * @param filePath 文件路径
     * @return std::string 文件扩展名
     */
    std::string extractExtension(const std::string& filePath) const;

    /**
     * @brief 检查类型是否在列表中（不区分大小写）
     * @param type 文件类型
     * @param types 类型列表
     * @return bool 存在返回true
     */
    bool isTypeInList(const std::string& type, const std::vector<std::string>& types) const;
};

#endif // TYPEFILTER_H