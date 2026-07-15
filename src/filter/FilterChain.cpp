#include "FilterChain.h"

FilterChain::FilterChain(Mode mode) : mode_(mode) {
}

void FilterChain::addFilter(std::shared_ptr<IFileFilter> filter) {
    if (filter) {
        filters_.push_back(filter);
    }
}

void FilterChain::clearFilters() {
    filters_.clear();
}

size_t FilterChain::getFilterCount() const {
    return filters_.size();
}

bool FilterChain::accept(const FileInfo& fileInfo) const {
    if (filters_.empty()) {
        return true;  // 没有过滤器时，接受所有文件
    }

    if (mode_ == Mode::AND) {
        // AND模式：所有过滤器都必须接受
        for (const auto& filter : filters_) {
            if (!filter->accept(fileInfo)) {
                return false;
            }
        }
        return true;
    } else {
        // OR模式：任意一个过滤器接受即可
        for (const auto& filter : filters_) {
            if (filter->accept(fileInfo)) {
                return true;
            }
        }
        return false;
    }
}

std::string FilterChain::getName() const {
    return "FilterChain";
}

std::string FilterChain::getDescription() const {
    std::string desc = "过滤器链 (" + std::to_string(filters_.size()) + " 个过滤器, ";
    desc += (mode_ == Mode::AND) ? "AND模式" : "OR模式";
    desc += ")";
    return desc;
}