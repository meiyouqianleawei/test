#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace DataBackup {

/**
 * 实时备份监控配置
 */
struct RealtimeBackupConfig {
    std::string sourcePath;           // 监控的源目录
    std::string targetPath;           // 备份目标目录
    bool enableEncryption = false;    // 是否启用加密
    std::string encryptionPassword;   // 加密密码
    bool includeSubdirectories = true; // 是否包含子目录
    std::vector<std::string> fileFilters; // 文件过滤器（扩展名，如 "txt"、"doc"）

    // 启动时先做一次全量同步(默认关，由 GUI 显式打开)
    bool initialFullSync = false;
    // 源侧删除/重命名时,同步删除目标侧对应文件（双向镜像）
    bool mirrorDelete = true;
};

/**
 * 文件变化事件类型
 */
enum class FileChangeType {
    CREATED,    // 文件创建
    MODIFIED,   // 文件修改
    DELETED,    // 文件删除
    RENAMED     // 文件重命名
};

/**
 * 文件变化事件
 */
struct FileChangeEvent {
    FileChangeType type;      // 变化类型
    std::string filePath;     // 文件路径
    std::string oldFilePath;  // 旧文件路径（重命名时有效）
    uint64_t timestamp;       // 时间戳
};

/**
 * 实时备份统计信息
 */
struct RealtimeBackupStats {
    uint32_t filesMonitored = 0;      // 监控的文件数
    uint32_t filesBackedUp = 0;       // 已备份文件数
    uint32_t filesDeleted = 0;        // 已同步删除的文件数
    uint32_t backupFailures = 0;      // 备份失败次数
    uint64_t totalBytesBackedUp = 0;  // 总备份字节数
    uint64_t startTime = 0;           // 启动时间
};

/**
 * 实时备份状态
 */
enum class RealtimeBackupStatus {
    IDLE,       // 空闲
    RUNNING,    // 运行中
    PAUSED,     // 已暂停
    ERROR_STATE // 错误状态（避免与Windows.h中的ERROR宏冲突）
};

} // namespace DataBackup