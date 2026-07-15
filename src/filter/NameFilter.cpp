#include "NameFilter.h"
#include <algorithm>
#include <regex>

NameFilter::NameFilter(const std::string& pattern, bool useRegex)
    : pattern_(pattern), useRegex_(useRegex) {
}

void NameFilter::setPattern(const std::string& pattern) {
    pattern_ = pattern;
}

bool NameFilter::accept(const FileInfo& fileInfo) const {
    std::string fileName = extractFileName(fileInfo.path.string());

    // 转换为小写（不区分大小写匹配）
    std::string fileNameLower = fileName;
    std::transform(fileNameLower.begin(), fileNameLower.end(), fileNameLower.begin(), ::tolower);

    if (useRegex_) {
        // 正则表达式匹配
        try {
            std::regex regex(pattern_, std::regex::icase);
            return std::regex_search(fileName, regex);
        } catch (const std::regex_error&) {
            return false;  // 正则表达式无效
        }
    } else {
        // 通配符匹配
        std::string patternLower = pattern_;
        std::transform(patternLower.begin(), patternLower.end(), patternLower.begin(), ::tolower);
        return wildcardMatch(fileNameLower, patternLower);
    }
}

bool NameFilter::wildcardMatch(const std::string& text, const std::string& pattern) const {
    size_t textLen = text.length();
    size_t patternLen = pattern.length();

    // 动态规划表：dp[i][j] 表示 text[0..i) 和 pattern[0..j) 是否匹配
    std::vector<std::vector<bool>> dp(textLen + 1, std::vector<bool>(patternLen + 1, false));

    // 空字符串和空模式匹配
    dp[0][0] = true;

    // 处理pattern以*开头的情况
    for (size_t j = 1; j <= patternLen; ++j) {
        if (pattern[j - 1] == '*') {
            dp[0][j] = dp[0][j - 1];
        }
    }

    // 填充dp表
    for (size_t i = 1; i <= textLen; ++i) {
        for (size_t j = 1; j <= patternLen; ++j) {
            if (pattern[j - 1] == '*') {
                // '*'可以匹配0个或多个字符
                dp[i][j] = dp[i][j - 1] || dp[i - 1][j];
            } else if (pattern[j - 1] == '?' || pattern[j - 1] == text[i - 1]) {
                // '?'匹配任意单个字符，或者字符相等
                dp[i][j] = dp[i - 1][j - 1];
            }
        }
    }

    return dp[textLen][patternLen];
}

std::string NameFilter::extractFileName(const std::string& filePath) const {
    size_t lastSlash = filePath.find_last_of("/\\");
    if (lastSlash == std::string::npos) {
        return filePath;
    }
    return filePath.substr(lastSlash + 1);
}

std::string NameFilter::getName() const {
    return "NameFilter";
}

std::string NameFilter::getDescription() const {
    return "文件名过滤器 [模式: " + pattern_ +
           (useRegex_ ? " (正则)" : " (通配符)") + "]";
}