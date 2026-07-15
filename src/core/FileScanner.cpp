#include "FileScanner.h"
#include <iostream>
#include <fstream>

namespace fs = std::filesystem;

/**
 * @brief 默认构造函数
 */
FileScanner::FileScanner()
    : followSymlinks_(false)
    , scanAlgorithm_(ScanAlgorithm::BREADTH_FIRST) {
}

/**
 * @brief 析构函数
 */
FileScanner::~FileScanner() {
}

/**
 * @brief 扫描指定目录树
 */
bool FileScanner::scan(const fs::path& rootPath, std::vector<FileInfo>& files) {
    // 重置状态
    reset();

    // 检查目录是否存在
    if (!fs::exists(rootPath)) {
        return false;
    }

    if (!fs::is_directory(rootPath)) {
        return false;
    }

    // 根据选择的算法执行扫描
    if (scanAlgorithm_ == ScanAlgorithm::BREADTH_FIRST) {
        scanBreadthFirst(rootPath, files);
    } else {
        scanDepthFirst(rootPath, files);
    }

    return statistics_.errorsCount == 0;
}

/**
 * @brief 设置是否跟随符号链接
 */
void FileScanner::setFollowSymlinks(bool follow) {
    followSymlinks_ = follow;
}

/**
 * @brief 获取是否跟随符号链接的设置
 */
bool FileScanner::getFollowSymlinks() const {
    return followSymlinks_;
}

/**
 * @brief 设置扫描算法
 */
void FileScanner::setScanAlgorithm(ScanAlgorithm algorithm) {
    scanAlgorithm_ = algorithm;
}

/**
 * @brief 获取扫描统计信息
 */
ScanStatistics FileScanner::getStatistics() const {
    return statistics_;
}

/**
 * @brief 重置扫描器和统计信息
 */
void FileScanner::reset() {
    hardlinksMap_.clear();
    statistics_ = ScanStatistics();
}

#ifdef _WIN32
/**
 * @brief 提取 Windows 文件元数据（重点功能）
 *
 * 使用 Windows API 获取文件的详细信息：
 * - 使用 CreateFileW 打开文件
 * - 使用 GetFileInformationByHandle 获取文件信息
 * - 提取时间戳、文件属性、硬链接信息
 */
bool FileScanner::extractWindowsMetadata(const fs::path& path, FileInfo& fileInfo) {
    // 使用 FILE_FLAG_OPEN_REPARSE_POINT 标志，确保正确处理符号链接
    HANDLE hFile = CreateFileW(
        path.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_OPEN_REPARSE_POINT | FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        statistics_.errorsCount++;
        return false;
    }

    BY_HANDLE_FILE_INFORMATION handleInfo;
    if (!GetFileInformationByHandle(hFile, &handleInfo)) {
        CloseHandle(hFile);
        statistics_.errorsCount++;
        return false;
    }

    // 转换 FILETIME 到 uint64_t
    ULARGE_INTEGER uliCreation, uliWrite;
    uliCreation.LowPart = handleInfo.ftCreationTime.dwLowDateTime;
    uliCreation.HighPart = handleInfo.ftCreationTime.dwHighDateTime;
    uliWrite.LowPart = handleInfo.ftLastWriteTime.dwLowDateTime;
    uliWrite.HighPart = handleInfo.ftLastWriteTime.dwHighDateTime;

    fileInfo.creationTime = uliCreation.QuadPart;
    fileInfo.lastWriteTime = uliWrite.QuadPart;

    // 提取文件属性
    fileInfo.attributes = handleInfo.dwFileAttributes;

    // 提取硬链接信息
    fileInfo.hardlinkCount = handleInfo.nNumberOfLinks;

    CloseHandle(hFile);
    return true;
}

/**
 * @brief 检查是否为硬链接的重复引用（重点功能）
 *
 * 使用 Windows API 获取文件唯一标识：
 * - 使用 GetFileInformationByHandle 获取 nFileIndexHigh/Low
 * - 使用 dwVolumeSerialNumber 作为卷标识
 * - 通过哈希表判断是否已扫描过该文件
 */
bool FileScanner::checkHardlink(const fs::path& path, FileInfo& fileInfo) {
    HANDLE hFile = CreateFileW(
        path.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_OPEN_REPARSE_POINT | FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        return false;
    }

    BY_HANDLE_FILE_INFORMATION handleInfo;
    if (!GetFileInformationByHandle(hFile, &handleInfo)) {
        CloseHandle(hFile);
        return false;
    }

    CloseHandle(hFile);

    // 构造唯一文件标识键
    HardlinkKey key;
    key.volumeSerial = handleInfo.dwVolumeSerialNumber;
    key.fileIndex = (static_cast<uint64_t>(handleInfo.nFileIndexHigh) << 32) |
                    static_cast<uint64_t>(handleInfo.nFileIndexLow);

    // 检查是否已扫描过该文件（硬链接）
    auto it = hardlinksMap_.find(key);
    if (it != hardlinksMap_.end()) {
        // 这是硬链接的重复引用，记录但不重复备份
        fileInfo.isHardlink = true;
        fileInfo.hardlinkTarget = it->second;
        fileInfo.hardlinkCount = handleInfo.nNumberOfLinks;
        statistics_.hardlinksFound++;
        return true;  // 返回 true 表示是重复引用
    }

    // 首次遇到该文件，记录唯一标识
    hardlinksMap_[key] = path;
    fileInfo.hardlinkCount = handleInfo.nNumberOfLinks;

    return false;  // 返回 false 表示不是重复引用
}
#endif

/**
 * @brief 检查是否为符号链接
 */
bool FileScanner::checkSymlink(const fs::path& path, FileInfo& fileInfo) {
    try {
        if (fs::is_symlink(path)) {
            fileInfo.isSymlink = true;

            // 读取符号链接目标路径
            fs::path target = fs::read_symlink(path);
            fileInfo.symlinkTarget = target;

            statistics_.symlinksFound++;
            return true;
        }
    } catch (const fs::filesystem_error& e) {
        // 处理异常但不中断扫描
        statistics_.errorsCount++;
        std::cerr << "Error checking symlink: " << e.what() << std::endl;
    }

    return false;
}

/**
 * @brief 广度优先扫描实现（推荐）
 *
 * 使用队列实现广度优先遍历，避免递归栈溢出
 */
void FileScanner::scanBreadthFirst(const fs::path& rootPath, std::vector<FileInfo>& files) {
    std::queue<fs::path> dirQueue;
    dirQueue.push(rootPath);

    while (!dirQueue.empty()) {
        fs::path currentDir = dirQueue.front();
        dirQueue.pop();

        statistics_.totalDirectories++;

        try {
            // 遍历当前目录
            for (const auto& entry : fs::directory_iterator(currentDir)) {
                if (entry.is_directory()) {
                    // 处理目录：加入队列继续扫描
                    processDirectory(entry.path(), dirQueue);
                } else if (entry.is_regular_file() || entry.is_symlink()) {
                    // 处理文件
                    processFile(entry.path(), files);
                }
            }
        } catch (const fs::filesystem_error& e) {
            // 处理异常但不中断扫描（权限不足、文件被占用等）
            statistics_.errorsCount++;
            std::cerr << "Error scanning directory: " << e.what() << std::endl;
        }
    }
}

/**
 * @brief 深度优先扫描实现
 *
 * 使用递归实现深度优先遍历
 */
void FileScanner::scanDepthFirst(const fs::path& currentPath, std::vector<FileInfo>& files) {
    try {
        for (const auto& entry : fs::recursive_directory_iterator(currentPath)) {
            if (entry.is_directory()) {
                statistics_.totalDirectories++;
            } else if (entry.is_regular_file() || entry.is_symlink()) {
                processFile(entry.path(), files);
            }
        }
    } catch (const fs::filesystem_error& e) {
        statistics_.errorsCount++;
        std::cerr << "Error scanning directory: " << e.what() << std::endl;
    }
}

/**
 * @brief 处理单个文件
 *
 * 收集文件信息，处理硬链接和符号链接
 */
void FileScanner::processFile(const fs::path& path, std::vector<FileInfo>& files) {
    FileInfo fileInfo(path);

    // 检查符号链接
    if (checkSymlink(path, fileInfo)) {
        if (followSymlinks_) {
            // 跟随符号链接，扫描目标文件
            try {
                fs::path target = fs::read_symlink(path);
                if (fs::exists(target) && fs::is_regular_file(target)) {
                    fileInfo.path = target;
                    fileInfo.size = fs::file_size(target);
                }
            } catch (const fs::filesystem_error&) {
                statistics_.errorsCount++;
            }
        }
        files.push_back(fileInfo);
        statistics_.totalFiles++;
        return;
    }

#ifdef _WIN32
    // 检查硬链接（Windows）
    if (checkHardlink(path, fileInfo)) {
        // 这是硬链接的重复引用，记录但不重复备份
        files.push_back(fileInfo);
        statistics_.totalFiles++;
        return;
    }

    // 提取 Windows 元数据
    extractWindowsMetadata(path, fileInfo);
#endif

    // 提取文件大小
    try {
        fileInfo.size = fs::file_size(path);
        statistics_.totalSize += fileInfo.size;
    } catch (const fs::filesystem_error&) {
        statistics_.errorsCount++;
    }

    files.push_back(fileInfo);
    statistics_.totalFiles++;
}

/**
 * @brief 处理目录
 *
 * 将目录加入扫描队列（广度优先扫描使用）
 */
void FileScanner::processDirectory(const fs::path& path, std::queue<fs::path>& dirQueue) {
    // 检查是否为符号链接目录
    try {
        if (fs::is_symlink(path)) {
            FileInfo fileInfo(path);
            fileInfo.isSymlink = true;
            fileInfo.symlinkTarget = fs::read_symlink(path);
            statistics_.symlinksFound++;

            if (followSymlinks_) {
                // 跟随符号链接，将目标目录加入队列
                fs::path target = fs::read_symlink(path);
                if (fs::exists(target) && fs::is_directory(target)) {
                    dirQueue.push(target);
                }
            }
        } else {
            // 普通目录，加入队列
            dirQueue.push(path);
        }
    } catch (const fs::filesystem_error&) {
        statistics_.errorsCount++;
    }
}