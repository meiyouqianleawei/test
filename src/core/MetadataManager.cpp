#include "MetadataManager.h"
#include <windows.h>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>

/**
 * @brief 构造函数
 */
MetadataManager::MetadataManager() {
}

/**
 * @brief 析构函数
 */
MetadataManager::~MetadataManager() {
}

/**
 * @brief 从文件提取元数据
 * @param filePath 文件路径
 * @return FileInfo 包含完整元数据的文件信息对象
 */
FileInfo MetadataManager::extractMetadata(const fs::path& filePath) {
    FileInfo info;

    if (!fs::exists(filePath)) {
        // 文件不存在，返回空的 FileInfo
        return info;
    }

    if (!getFileMetadataWindowsAPI(filePath, info)) {
        // Windows API 失败，使用 std::filesystem 作为备选
        info.path = filePath;
        info.size = fs::is_directory(filePath) ? 0 : fs::file_size(filePath);
        info.isDirectory = fs::is_directory(filePath);
        info.isHardlink = false;
        info.isSymlink = fs::is_symlink(filePath);

        // 使用 std::filesystem 获取时间（精度较低）
        auto ftime = fs::last_write_time(filePath);
        auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now()
        );
        auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(
            sctp.time_since_epoch()
        ).count();
        info.lastWriteTime = timestamp * 10000000ULL + 116444736000000000ULL;
        info.creationTime = info.lastWriteTime;
        info.lastAccessTime = info.lastWriteTime;

        // 获取文件属性
        DWORD attrs = GetFileAttributesW(filePath.wstring().c_str());
        if (attrs != INVALID_FILE_ATTRIBUTES) {
            info.attributes = attrs;
        } else {
            info.attributes = FILE_ATTRIBUTE_NORMAL;
        }
    }

    return info;
}

/**
 * @brief 设置文件元数据
 * @param info 包含要设置的元数据的 FileInfo 对象
 * @return bool 成功返回 true，失败返回 false
 */
bool MetadataManager::setMetadata(const FileInfo& info) {
    return setMetadata(info, info.path);
}

/**
 * @brief 设置指定文件的元数据
 * @param info 包含元数据的 FileInfo 对象
 * @param targetPath 目标文件路径
 * @return bool 成功返回 true，失败返回 false
 */
bool MetadataManager::setMetadata(const FileInfo& info, const fs::path& targetPath) {
    if (!fs::exists(targetPath)) {
        return false;
    }

    bool success = true;

    // 设置时间戳
    if (info.creationTime > 0 || info.lastWriteTime > 0 || info.lastAccessTime > 0) {
        if (!setFileTimestampsWindowsAPI(targetPath,
                                         info.creationTime,
                                         info.lastWriteTime,
                                         info.lastAccessTime)) {
            success = false;
        }
    }

    // 设置文件属性
    if (info.attributes != FILE_ATTRIBUTE_NORMAL) {
        if (!setFileAttributesWindowsAPI(targetPath, info.attributes)) {
            success = false;
        }
    }

    return success;
}

/**
 * @brief 批量提取元数据
 * @param filePaths 文件路径列表
 * @return std::vector<FileInfo> 包含所有文件元数据的列表
 */
std::vector<FileInfo> MetadataManager::extractBatchMetadata(
    const std::vector<fs::path>& filePaths) {
    std::vector<FileInfo> infos;
    infos.reserve(filePaths.size());

    for (const auto& filePath : filePaths) {
        infos.push_back(extractMetadata(filePath));
    }

    return infos;
}

/**
 * @brief 序列化元数据到字符串（JSON 格式）
 * @param info 文件信息对象
 * @return std::string 序列化的元数据字符串
 */
std::string MetadataManager::serializeMetadata(const FileInfo& info) {
    std::ostringstream oss;

    oss << "{\n";
    oss << "  \"path\": \"" << info.path.string() << "\",\n";
    oss << "  \"size\": " << info.size << ",\n";
    oss << "  \"isDirectory\": " << (info.isDirectory ? "true" : "false") << ",\n";
    oss << "  \"isHardlink\": " << (info.isHardlink ? "true" : "false") << ",\n";
    oss << "  \"isSymlink\": " << (info.isSymlink ? "true" : "false") << ",\n";
    oss << "  \"creationTime\": " << info.creationTime << ",\n";
    oss << "  \"lastWriteTime\": " << info.lastWriteTime << ",\n";
    oss << "  \"lastAccessTime\": " << info.lastAccessTime << ",\n";
    oss << "  \"attributes\": " << info.attributes << ",\n";
    oss << "  \"permissions\": \"" << info.permissions << "\"\n";
    oss << "}\n";

    return oss.str();
}

/**
 * @brief 从字符串反序列化元数据
 * @param serialized 序列化的元数据字符串
 * @return FileInfo 反序列化的文件信息对象
 */
FileInfo MetadataManager::deserializeMetadata(const std::string& serialized) {
    FileInfo info;

    // 简单的 JSON 解析（不依赖第三方库）
    auto findValue = [&serialized](const std::string& key) -> std::string {
        std::string searchKey = "\"" + key + "\"";
        size_t pos = serialized.find(searchKey);
        if (pos == std::string::npos) return "";

        pos = serialized.find(":", pos);
        if (pos == std::string::npos) return "";

        pos = serialized.find_first_not_of(" \t\n", pos + 1);
        if (pos == std::string::npos) return "";

        // 查找值结束位置
        size_t endPos;
        if (serialized[pos] == '"') {
            // 字符串值
            endPos = serialized.find('"', pos + 1);
            if (endPos == std::string::npos) return "";
            return serialized.substr(pos + 1, endPos - pos - 1);
        } else if (serialized[pos] == 't' || serialized[pos] == 'f') {
            // boolean 值
            endPos = serialized.find_first_of(",\n}", pos);
            if (endPos == std::string::npos) return "";
            return serialized.substr(pos, endPos - pos);
        } else {
            // 数字值
            endPos = serialized.find_first_of(",\n}", pos);
            if (endPos == std::string::npos) return "";
            return serialized.substr(pos, endPos - pos);
        }
    };

    info.path = findValue("path");
    info.size = std::stoull(findValue("size"));
    info.isDirectory = (findValue("isDirectory") == "true");
    info.isHardlink = (findValue("isHardlink") == "true");
    info.isSymlink = (findValue("isSymlink") == "true");
    info.creationTime = std::stoull(findValue("creationTime"));
    info.lastWriteTime = std::stoull(findValue("lastWriteTime"));
    info.lastAccessTime = std::stoull(findValue("lastAccessTime"));
    info.attributes = std::stoul(findValue("attributes"));
    info.permissions = findValue("permissions");

    return info;
}

/**
 * @brief 保存元数据到文件
 * @param info 文件信息对象
 * @param metadataFilePath 元数据文件路径
 * @return bool 成功返回 true，失败返回 false
 */
bool MetadataManager::saveMetadataToFile(const FileInfo& info,
                                        const fs::path& metadataFilePath) {
    try {
        std::string serialized = serializeMetadata(info);
        std::ofstream file(metadataFilePath, std::ios::binary);
        if (!file.is_open()) {
            return false;
        }

        file << serialized;
        file.close();
        return true;
    } catch (...) {
        return false;
    }
}

/**
 * @brief 从文件加载元数据
 * @param metadataFilePath 元数据文件路径
 * @return FileInfo 加载的文件信息对象
 */
FileInfo MetadataManager::loadMetadataFromFile(const fs::path& metadataFilePath) {
    try {
        std::ifstream file(metadataFilePath, std::ios::binary);
        if (!file.is_open()) {
            return FileInfo();
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();

        return deserializeMetadata(buffer.str());
    } catch (...) {
        return FileInfo();
    }
}

/**
 * @brief 从 Windows FILETIME 转换为 uint64_t
 * @param filetime Windows FILETIME 结构
 * @return uint64_t 100 纳秒单位的时间戳
 */
uint64_t MetadataManager::convertFiletimeToUint64(const FILETIME& filetime) {
    ULARGE_INTEGER uli;
    uli.LowPart = filetime.dwLowDateTime;
    uli.HighPart = filetime.dwHighDateTime;
    return uli.QuadPart;
}

/**
 * @brief 从 uint64_t 转换为 Windows FILETIME
 * @param timestamp 100 纳秒单位的时间戳
 * @return FILETIME Windows FILETIME 结构
 */
FILETIME MetadataManager::convertUint64ToFiletime(uint64_t timestamp) {
    ULARGE_INTEGER uli;
    uli.QuadPart = timestamp;
    FILETIME filetime;
    filetime.dwLowDateTime = uli.LowPart;
    filetime.dwHighDateTime = uli.HighPart;
    return filetime;
}

/**
 * @brief 使用 Windows API 获取文件元数据
 * @param filePath 文件路径
 * @param info 输出的文件信息对象
 * @return bool 成功返回 true，失败返回 false
 */
bool MetadataManager::getFileMetadataWindowsAPI(const fs::path& filePath, FileInfo& info) {
    WIN32_FILE_ATTRIBUTE_DATA attrData;

    // 使用 GetFileAttributesExW 获取文件属性和元数据
    if (!GetFileAttributesExW(filePath.wstring().c_str(),
                             GetFileExInfoStandard,
                             &attrData)) {
        return false;
    }

    info.path = filePath;
    info.attributes = attrData.dwFileAttributes;
    info.isDirectory = (attrData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

    // 检查是否为符号链接
    info.isSymlink = (attrData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;

    // 获取文件大小（非目录）
    if (!info.isDirectory) {
        ULARGE_INTEGER fileSize;
        fileSize.LowPart = attrData.nFileSizeLow;
        fileSize.HighPart = attrData.nFileSizeHigh;
        info.size = fileSize.QuadPart;
    } else {
        info.size = 0;
    }

    // 转换时间戳
    info.creationTime = convertFiletimeToUint64(attrData.ftCreationTime);
    info.lastWriteTime = convertFiletimeToUint64(attrData.ftLastWriteTime);
    info.lastAccessTime = convertFiletimeToUint64(attrData.ftLastAccessTime);

    // 硬链接检测需要使用 GetFileInformationByHandle
    if (!info.isDirectory && !info.isSymlink) {
        HANDLE hFile = CreateFileW(
            filePath.wstring().c_str(),
            GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            nullptr,
            OPEN_EXISTING,
            FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS,
            nullptr
        );

        if (hFile != INVALID_HANDLE_VALUE) {
            BY_HANDLE_FILE_INFORMATION handleInfo;
            if (GetFileInformationByHandle(hFile, &handleInfo)) {
                info.isHardlink = handleInfo.nNumberOfLinks > 1;

                // 构建硬链接唯一标识
                std::ostringstream oss;
                oss << handleInfo.dwVolumeSerialNumber << "-"
                    << handleInfo.nFileIndexHigh << "-" << handleInfo.nFileIndexLow;
                info.hardlinkTarget = oss.str();
                info.hardlinkCount = handleInfo.nNumberOfLinks;
            }
            CloseHandle(hFile);
        }
    }

    // 设置权限信息（Windows 下简化处理）
    info.permissions = "read-write";
    if (attrData.dwFileAttributes & FILE_ATTRIBUTE_READONLY) {
        info.permissions = "read-only";
    }

    return true;
}

/**
 * @brief 使用 Windows API 设置文件时间戳
 * @param filePath 文件路径
 * @param creationTime 创建时间
 * @param lastWriteTime 最后写入时间
 * @param lastAccessTime 最后访问时间
 * @return bool 成功返回 true，失败返回 false
 */
bool MetadataManager::setFileTimestampsWindowsAPI(const fs::path& filePath,
                                                  uint64_t creationTime,
                                                  uint64_t lastWriteTime,
                                                  uint64_t lastAccessTime) {
    HANDLE hFile = CreateFileW(
        filePath.wstring().c_str(),
        FILE_WRITE_ATTRIBUTES,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,
        nullptr
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        return false;
    }

    FILETIME ftCreation = convertUint64ToFiletime(creationTime);
    FILETIME ftWrite = convertUint64ToFiletime(lastWriteTime);
    FILETIME ftAccess = convertUint64ToFiletime(lastAccessTime);

    BOOL success = SetFileTime(
        hFile,
        creationTime > 0 ? &ftCreation : nullptr,
        lastAccessTime > 0 ? &ftAccess : nullptr,
        lastWriteTime > 0 ? &ftWrite : nullptr
    );

    CloseHandle(hFile);
    return success != 0;
}

/**
 * @brief 使用 Windows API 设置文件属性
 * @param filePath 文件路径
 * @param attributes 文件属性
 * @return bool 成功返回 true，失败返回 false
 */
bool MetadataManager::setFileAttributesWindowsAPI(const fs::path& filePath, DWORD attributes) {
    BOOL success = SetFileAttributesW(filePath.wstring().c_str(), attributes);
    return success != 0;
}