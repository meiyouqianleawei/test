#include "SizeFilter.h"
#include <sstream>
#include <iomanip>
#include <algorithm>

SizeFilter::SizeFilter(uint64_t minSize, uint64_t maxSize)
    : minSize_(minSize), maxSize_(maxSize) {
}

void SizeFilter::setSizeRange(uint64_t minSize, uint64_t maxSize) {
    minSize_ = minSize;
    maxSize_ = maxSize;
}

SizeFilter SizeFilter::fromReadable(const std::string& minSize, const std::string& maxSize) {
    return SizeFilter(parseSize(minSize), parseSize(maxSize));
}

bool SizeFilter::accept(const FileInfo& fileInfo) const {
    // 目录不过滤大小
    if (fileInfo.isDirectory) {
        return true;
    }

    uint64_t fileSize = fileInfo.size;

    // 0表示不限制
    bool aboveMin = (minSize_ == 0) || (fileSize >= minSize_);
    bool belowMax = (maxSize_ == 0) || (fileSize <= maxSize_);

    return aboveMin && belowMax;
}

uint64_t SizeFilter::parseSize(const std::string& sizeStr) {
    if (sizeStr.empty() || sizeStr == "0" || sizeStr == "0B") {
        return 0;
    }

    std::string str = sizeStr;
    std::transform(str.begin(), str.end(), str.begin(), ::toupper);

    // 提取数字部分
    std::string numStr;
    std::string unit;
    for (char c : str) {
        if (std::isdigit(c) || c == '.' || c == '-') {
            numStr += c;
        } else {
            unit += c;
            break;
        }
    }

    if (numStr.empty()) {
        return 0;
    }

    double size = std::stod(numStr);

    // 转换单位
    if (unit == "KB" || unit == "K") {
        size *= 1024;
    } else if (unit == "MB" || unit == "M") {
        size *= 1024 * 1024;
    } else if (unit == "GB" || unit == "G") {
        size *= 1024 * 1024 * 1024;
    } else if (unit == "TB" || unit == "T") {
        size *= 1024.0 * 1024 * 1024 * 1024;
    }
    // 默认是字节（B）

    return static_cast<uint64_t>(size);
}

std::string SizeFilter::formatSize(uint64_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unitIndex = 0;
    double size = static_cast<double>(bytes);

    while (size >= 1024.0 && unitIndex < 4) {
        size /= 1024.0;
        unitIndex++;
    }

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << size << units[unitIndex];
    return oss.str();
}

std::string SizeFilter::getName() const {
    return "SizeFilter";
}

std::string SizeFilter::getDescription() const {
    std::string desc = "大小过滤器";

    if (minSize_ > 0 || maxSize_ > 0) {
        desc += " [范围: ";
        if (minSize_ > 0) desc += formatSize(minSize_);
        else desc += "0B";

        desc += " - ";

        if (maxSize_ > 0) desc += formatSize(maxSize_);
        else desc += "无限制";

        desc += "]";
    }

    return desc;
}