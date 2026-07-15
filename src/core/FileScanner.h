#ifndef FILESCANNER_H
#define FILESCANNER_H

#include <filesystem>
#include <vector>
#include <queue>
#include <unordered_map>
#include <cstdint>
#include "FileInfo.h"

#ifdef _WIN32
#include <windows.h>
#endif

namespace fs = std::filesystem;

/**
 * @brief 扫描算法类型
 */
enum class ScanAlgorithm {
    BREADTH_FIRST,  ///< 广度优先遍历（推荐，避免递归栈溢出）
    DEPTH_FIRST     ///< 深度优先遍历
};

/**
 * @brief 扫描统计信息结构体
 */
struct ScanStatistics {
    uint32_t totalFiles;         ///< 扫描到的文件总数
    uint32_t totalDirectories;   ///< 扫描到的目录总数
    uint32_t hardlinksFound;     ///< 发现的硬链接数（重复引用）
    uint32_t symlinksFound;      ///< 发现的符号链接数
    uint64_t totalSize;          ///< 文件总大小（字节）
    uint32_t errorsCount;        ///< 错误计数

    ScanStatistics()
        : totalFiles(0)
        , totalDirectories(0)
        , hardlinksFound(0)
        , symlinksFound(0)
        , totalSize(0)
        , errorsCount(0) {}
};

/**
 * @brief 文件扫描器类
 *
 * 负责扫描指定目录树，收集文件信息，处理硬链接和符号链接。
 * 使用广度优先遍历算法，避免递归栈溢出。
 * 支持 Windows API 识别硬链接和符号链接。
 *
 * **重点功能**（老师强调）：
 * - 硬链接识别：使用 GetFileInformationByHandle 获取唯一文件标识
 * - 软链接处理：使用 FILE_FLAG_OPEN_REPARSE_POINT 标志识别和处理
 * - 链接计数：记录硬链接引用计数，只备份第一个引用
 * - 扫描算法：采用广度优先策略，避免递归栈溢出
 */
class FileScanner {
public:
    /**
     * @brief 默认构造函数
     */
    FileScanner();

    /**
     * @brief 析构函数
     */
    ~FileScanner();

    /**
     * @brief 扫描指定目录树
     *
     * @param rootPath 要扫描的根目录路径
     * @param files 输出参数，存储扫描到的文件信息列表
     * @return true 扫描成功
     * @return false 扫描失败（目录不存在或发生错误）
     */
    bool scan(const fs::path& rootPath, std::vector<FileInfo>& files);

    /**
     * @brief 设置是否跟随符号链接
     *
     * @param follow true 表示跟随符号链接，扫描目标文件
     *               false 表示不跟随，将符号链接作为单独条目记录
     */
    void setFollowSymlinks(bool follow);

    /**
     * @brief 获取是否跟随符号链接的设置
     *
     * @return 当前设置
     */
    bool getFollowSymlinks() const;

    /**
     * @brief 设置扫描算法
     *
     * @param algorithm 扫描算法类型（广度优先或深度优先）
     */
    void setScanAlgorithm(ScanAlgorithm algorithm);

    /**
     * @brief 获取扫描统计信息
     *
     * @return 扫描统计信息结构体
     */
    ScanStatistics getStatistics() const;

    /**
     * @brief 重置扫描器和统计信息
     */
    void reset();

private:
    // 硬链接映射表：存储已扫描的文件唯一标识
    // Key: (volumeSerial, fileIndex), Value: 文件路径
    struct HardlinkKey {
        uint64_t volumeSerial;
        uint64_t fileIndex;

        bool operator==(const HardlinkKey& other) const {
            return volumeSerial == other.volumeSerial && fileIndex == other.fileIndex;
        }
    };

    // 自定义哈希函数
    struct HardlinkKeyHash {
        size_t operator()(const HardlinkKey& key) const {
            return std::hash<uint64_t>()(key.volumeSerial) ^ (std::hash<uint64_t>()(key.fileIndex) << 1);
        }
    };

    std::unordered_map<HardlinkKey, fs::path, HardlinkKeyHash> hardlinksMap_;

    // 扫描选项
    bool followSymlinks_;           ///< 是否跟随符号链接
    ScanAlgorithm scanAlgorithm_;   ///< 扫描算法

    // 统计信息
    ScanStatistics statistics_;

#ifdef _WIN32
    /**
     * @brief 提取 Windows 文件元数据
     *
     * 使用 Windows API 获取文件的详细信息，包括：
     * - 创建时间、修改时间
     * - 文件属性
     * - 硬链接信息（nFileIndexHigh/Low, nNumberOfLinks）
     *
     * @param path 文件路径
     * @param fileInfo 输出参数，存储文件信息
     * @return true 提取成功
     * @return false 提取失败
     */
    bool extractWindowsMetadata(const fs::path& path, FileInfo& fileInfo);

    /**
     * @brief 检查是否为硬链接的重复引用
     *
     * 使用 GetFileInformationByHandle 获取文件唯一标识，
     * 判断是否已经扫描过该文件（硬链接）。
     *
     * @param path 文件路径
     * @param fileInfo 输出参数，存储硬链接信息
     * @return true 是硬链接的重复引用（已扫描过）
     * @return false 不是硬链接，或首次遇到的硬链接
     */
    bool checkHardlink(const fs::path& path, FileInfo& fileInfo);
#endif

    /**
     * @brief 检查是否为符号链接
     *
     * @param path 文件路径
     * @param fileInfo 输出参数，存储符号链接信息
     * @return true 是符号链接
     * @return false 不是符号链接
     */
    bool checkSymlink(const fs::path& path, FileInfo& fileInfo);

    /**
     * @brief 广度优先扫描实现
     *
     * 使用队列实现广度优先遍历，避免递归栈溢出。
     *
     * @param rootPath 根目录路径
     * @param files 输出参数，存储文件信息
     */
    void scanBreadthFirst(const fs::path& rootPath, std::vector<FileInfo>& files);

    /**
     * @brief 深度优先扫描实现
     *
     * 使用递归实现深度优先遍历。
     *
     * @param currentPath 当前扫描路径
     * @param files 输出参数，存储文件信息
     */
    void scanDepthFirst(const fs::path& currentPath, std::vector<FileInfo>& files);

    /**
     * @brief 处理单个文件
     *
     * 收集文件信息，处理硬链接和符号链接。
     *
     * @param path 文件路径
     * @param files 输出参数，存储文件信息
     */
    void processFile(const fs::path& path, std::vector<FileInfo>& files);

    /**
     * @brief 处理目录
     *
     * 将目录加入扫描队列（广度优先）或继续递归（深度优先）。
     *
     * @param path 目录路径
     * @param dirQueue 目录队列（广度优先扫描使用）
     */
    void processDirectory(const fs::path& path, std::queue<fs::path>& dirQueue);
};

#endif // FILESCANNER_H