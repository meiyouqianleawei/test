#include "TypeFilter.h"
#include <algorithm>

TypeFilter::TypeFilter(const std::vector<std::string>& includeTypes,
                       const std::vector<std::string>& excludeTypes)
    : includeTypes_(includeTypes), excludeTypes_(excludeTypes) {
}

void TypeFilter::addIncludeType(const std::string& type) {
    includeTypes_.push_back(type);
}

void TypeFilter::addExcludeType(const std::string& type) {
    excludeTypes_.push_back(type);
}

void TypeFilter::clearIncludeTypes() {
    includeTypes_.clear();
}

void TypeFilter::clearExcludeTypes() {
    excludeTypes_.clear();
}

bool TypeFilter::accept(const FileInfo& fileInfo) const {
    // 目录不过滤类型
    if (fileInfo.isDirectory) {
        return true;
    }

    std::string extension = extractExtension(fileInfo.path.string());

    // 1. 检查排除类型（优先级最高）
    if (!excludeTypes_.empty() && isTypeInList(extension, excludeTypes_)) {
        return false;  // 排除
    }

    // 2. 检查包含类型
    if (includeTypes_.empty()) {
        return true;  // 没有包含类型限制，接受所有
    }

    return isTypeInList(extension, includeTypes_);
}

std::string TypeFilter::extractExtension(const std::string& filePath) const {
    size_t lastDot = filePath.find_last_of('.');
    if (lastDot == std::string::npos || lastDot == filePath.length() - 1) {
        return "";  // 没有扩展名
    }

    std::string ext = filePath.substr(lastDot + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext;
}

bool TypeFilter::isTypeInList(const std::string& type, const std::vector<std::string>& types) const {
    std::string typeLower = type;
    std::transform(typeLower.begin(), typeLower.end(), typeLower.begin(), ::tolower);

    for (const auto& t : types) {
        std::string tLower = t;
        std::transform(tLower.begin(), tLower.end(), tLower.begin(), ::tolower);

        if (typeLower == tLower) {
            return true;
        }
    }

    return false;
}

std::string TypeFilter::getName() const {
    return "TypeFilter";
}

std::string TypeFilter::getDescription() const {
    std::string desc = "类型过滤器";

    if (!includeTypes_.empty()) {
        desc += " [包含: ";
        for (size_t i = 0; i < includeTypes_.size(); ++i) {
            desc += includeTypes_[i];
            if (i < includeTypes_.size() - 1) desc += ", ";
        }
        desc += "]";
    }

    if (!excludeTypes_.empty()) {
        desc += " [排除: ";
        for (size_t i = 0; i < excludeTypes_.size(); ++i) {
            desc += excludeTypes_[i];
            if (i < excludeTypes_.size() - 1) desc += ", ";
        }
        desc += "]";
    }

    return desc;
}