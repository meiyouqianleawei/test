#ifndef METADATAMANAGER_H
#define METADATAMANAGER_H

#include <string>
#include <vector>
#include <filesystem>
#include "FileInfo.h"

namespace fs = std::filesystem;

/**
 * @brief 元数据管理器
 *
 * 负责提取、保存和恢复 Windows 文件元数据，包括：
 * - 文件时间戳（创建时间、修改时间、访问时间）
 * - 文件属性（只读、隐藏、系统等）
 * - 文件权限信息
 * - 元数据序列化和持久化
 */
class MetadataManager {
public:
    MetadataManager();
    ~MetadataManager();

    /**
     * @brief 从文件提取元数据
     * @param filePath 文件路径
     * @return FileInfo 包含完整元数据的文件信息对象
     */
    FileInfo extractMetadata(const fs::path& filePath);

    /**
     * @brief 设置文件元数据
     * @param info 包含要设置的元数据的 FileInfo 对象
     * @return bool 成功返回 true，失败返回 false
     */
    bool setMetadata(const FileInfo& info);

    /**
     * @brief 设置指定文件的元数据（不使用 info.path）
     * @param info 包含元数据的 FileInfo 对象
     * @param targetPath 目标文件路径
     * @return bool 成功返回 true，失败返回 false
     */
    bool setMetadata(const FileInfo& info, const fs::path& targetPath);

    /**
     * @brief 批量提取元数据
     * @param filePaths 文件路径列表
     * @return std::vector<FileInfo> 包含所有文件元数据的列表
     */
    std::vector<FileInfo> extractBatchMetadata(const std::vector<fs::path>& filePaths);

    /**
     * @brief 序列化元数据到字符串
     * @param info 文件信息对象
     * @return std::string 序列化的元数据字符串（JSON 格式）
     */
    std::string serializeMetadata(const FileInfo& info);

    /**
     * @brief 从字符串反序列化元数据
     * @param serialized 序列化的元数据字符串
     * @return FileInfo 反序列化的文件信息对象
     */
    FileInfo deserializeMetadata(const std::string& serialized);

    /**
     * @brief 保存元数据到文件
     * @param info 文件信息对象
     * @param metadataFilePath 元数据文件路径
     * @return bool 成功返回 true，失败返回 false
     */
    bool saveMetadataToFile(const FileInfo& info, const fs::path& metadataFilePath);

    /**
     * @brief 从文件加载元数据
     * @param metadataFilePath 元数据文件路径
     * @return FileInfo 加载的文件信息对象
     */
    FileInfo loadMetadataFromFile(const fs::path& metadataFilePath);

private:
    /**
     * @brief 从 Windows FILETIME 转换为 uint64_t
     * @param filetime Windows FILETIME 结构
     * @return uint64_t 100 纳秒单位的时间戳
     */
    uint64_t convertFiletimeToUint64(const FILETIME& filetime);

    /**
     * @brief 从 uint64_t 转换为 Windows FILETIME
     * @param timestamp 100 纳秒单位的时间戳
     * @return FILETIME Windows FILETIME 结构
     */
    FILETIME convertUint64ToFiletime(uint64_t timestamp);

    /**
     * @brief 使用 Windows API 获取文件元数据
     * @param filePath 文件路径
     * @param info 输出的文件信息对象
     * @return bool 成功返回 true，失败返回 false
     */
    bool getFileMetadataWindowsAPI(const fs::path& filePath, FileInfo& info);

    /**
     * @brief 使用 Windows API 设置文件时间戳
     * @param filePath 文件路径
     * @param creationTime 创建时间
     * @param lastWriteTime 最后写入时间
     * @param lastAccessTime 最后访问时间
     * @return bool 成功返回 true，失败返回 false
     */
    bool setFileTimestampsWindowsAPI(const fs::path& filePath,
                                     uint64_t creationTime,
                                     uint64_t lastWriteTime,
                                     uint64_t lastAccessTime);

    /**
     * @brief 使用 Windows API 设置文件属性
     * @param filePath 文件路径
     * @param attributes 文件属性
     * @return bool 成功返回 true，失败返回 false
     */
    bool setFileAttributesWindowsAPI(const fs::path& filePath, DWORD attributes);
};

#endif // METADATAMANAGER_H