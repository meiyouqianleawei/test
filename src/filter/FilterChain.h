#ifndef FILTERCHAIN_H
#define FILTERCHAIN_H

#include "IFileFilter.h"
#include <vector>
#include <memory>

/**
 * @brief 过滤器链（组合模式）
 *
 * 组合多个过滤器，按顺序判断。
 * 支持两种模式：
 * 1. AND模式（默认）：所有过滤器都返回true才接受
 * 2. OR模式：任意一个过滤器返回true就接受
 */
class FilterChain : public IFileFilter {
public:
    enum class Mode {
        AND,  // 所有过滤器都必须接受
        OR    // 任意一个过滤器接受即可
    };

    /**
     * @brief 构造过滤器链
     * @param mode 组合模式（AND或OR）
     */
    explicit FilterChain(Mode mode = Mode::AND);

    /**
     * @brief 添加过滤器到链中
     * @param filter 过滤器智能指针
     */
    void addFilter(std::shared_ptr<IFileFilter> filter);

    /**
     * @brief 清空所有过滤器
     */
    void clearFilters();

    /**
     * @brief 获取过滤器数量
     * @return size_t 过滤器数量
     */
    size_t getFilterCount() const;

    // 实现IFileFilter接口
    bool accept(const FileInfo& fileInfo) const override;
    std::string getName() const override;
    std::string getDescription() const override;

private:
    std::vector<std::shared_ptr<IFileFilter>> filters_;
    Mode mode_;
};

#endif // FILTERCHAIN_H