#include "PathFilter.h"
#include <algorithm>
#include <filesystem>

PathFilter::PathFilter(const std::vector<std::string>& includePaths,
                       const std::vector<std::string>& excludePaths)
    : includePaths_(includePaths), excludePaths_(excludePaths) {
}

void PathFilter::addIncludePath(const std::string& path) {
    includePaths_.push_back(path);
}

void PathFilter::addExcludePath(const std::string& path) {
    excludePaths_.push_back(path);
}

void PathFilter::clearIncludePaths() {
    includePaths_.clear();
}

void PathFilter::clearExcludePaths() {
    excludePaths_.clear();
}

bool PathFilter::accept(const FileInfo& fileInfo) const {
    std::string filePath = fileInfo.path.string();

    // 转换为统一格式（使用正斜杠）
    std::replace(filePath.begin(), filePath.end(), '\\', '/');

    // 1. 检查排除路径（优先级最高）
    for (const auto& excludePath : excludePaths_) {
        std::string normalized = excludePath;
        std::replace(normalized.begin(), normalized.end(), '\\', '/');

        if (matchesPrefix(filePath, normalized)) {
            return false;  // 排除
        }
    }

    // 2. 检查包含路径
    if (includePaths_.empty()) {
        return true;  // 没有包含路径限制，接受所有
    }

    for (const auto& includePath : includePaths_) {
        std::string normalized = includePath;
        std::replace(normalized.begin(), normalized.end(), '\\', '/');

        if (matchesPrefix(filePath, normalized)) {
            return true;  // 包含
        }
    }

    return false;  // 不在包含列表中
}

bool PathFilter::matchesPrefix(const std::string& path, const std::string& prefix) const {
    if (path.size() < prefix.size()) {
        return false;
    }

    // 不区分大小写比较
    std::string pathLower = path;
    std::string prefixLower = prefix;

    std::transform(pathLower.begin(), pathLower.end(), pathLower.begin(), ::tolower);
    std::transform(prefixLower.begin(), prefixLower.end(), prefixLower.begin(), ::tolower);

    return pathLower.compare(0, prefixLower.size(), prefixLower) == 0;
}

std::string PathFilter::getName() const {
    return "PathFilter";
}

std::string PathFilter::getDescription() const {
    std::string desc = "路径过滤器";

    if (!includePaths_.empty()) {
        desc += " [包含: ";
        for (size_t i = 0; i < includePaths_.size(); ++i) {
            desc += includePaths_[i];
            if (i < includePaths_.size() - 1) desc += ", ";
        }
        desc += "]";
    }

    if (!excludePaths_.empty()) {
        desc += " [排除: ";
        for (size_t i = 0; i < excludePaths_.size(); ++i) {
            desc += excludePaths_[i];
            if (i < excludePaths_.size() - 1) desc += ", ";
        }
        desc += "]";
    }

    return desc;
}