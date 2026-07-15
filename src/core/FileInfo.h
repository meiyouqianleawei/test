#ifndef FILEINFO_H
#define FILEINFO_H

#include <filesystem>
#include <string>
#include <cstdint>

#ifdef _WIN32
#include <windows.h>
#endif

namespace fs = std::filesystem;

/**
 * @brief 文件信息实体类
 *
 * 存储文件的基本属性和元数据信息，包括路径、大小、时间戳、权限等。
 * 支持硬链接和软链接的识别与处理。
 */
class FileInfo {
public:
    // 公共成员变量（便于直接访问和序列化）
    fs::path path;                    ///< 文件路径
    uint64_t size = 0;                ///< 文件大小（字节）

    bool isDirectory = false;         ///< 是否为目录
    bool isHardlink = false;          ///< 是否为硬链接的重复引用
    bool isSymlink = false;           ///< 是否为符号链接

    uint64_t creationTime = 0;        ///< 创建时间（100 纳秒单位，从 1601-01-01）
    uint64_t lastWriteTime = 0;       ///< 最后修改时间（100 纳秒单位）
    uint64_t lastAccessTime = 0;      ///< 最后访问时间（100 纳秒单位）

#ifdef _WIN32
    DWORD attributes = FILE_ATTRIBUTE_NORMAL;  ///< 文件属性（Windows）
#endif

    fs::path hardlinkTarget;           ///< 硬链接目标文件路径
    uint32_t hardlinkCount = 0;       ///< 硬链接引用计数

    fs::path symlinkTarget;           ///< 符号链接目标路径

    std::string permissions;          ///< 文件权限信息

    /**
     * @brief 默认构造函数
     */
    FileInfo() = default;

    /**
     * @brief 构造函数，初始化文件路径
     * @param filePath 文件路径
     */
    explicit FileInfo(const fs::path& filePath) : path(filePath) {}
};

#endif // FILEINFO_H