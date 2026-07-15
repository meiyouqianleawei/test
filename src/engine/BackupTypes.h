#pragma once
#include <string>
#include <cstdint>

namespace DataBackup {

/**
 * 备份阶段枚举
 */
enum class BackupPhase {
    IDLE,               // 空闲状态
    SCANNING,           // 扫描文件
    FILTERING,          // 过滤文件
    COMPRESSING,        // 压缩数据
    ENCRYPTING,         // 加密数据
    DECRYPTING,         // 解密数据  ✅ 新增
    DECOMPRESSING,      // 解压数据  ✅ 新增
    PACKING,            // 打包文件
    VERIFYING,          // 验证备份
    COMPLETED,          // 完成
    FAILED,             // 失败
    CANCELLED           // 已取消
};

/**
 * 备份进度类
 * 记录备份过程的实时进度信息
 */
struct BackupProgress {
    BackupPhase phase = BackupPhase::IDLE;  // 当前阶段
    std::string currentFile;                 // 当前处理的文件
    uint32_t filesProcessed = 0;             // 已处理文件数
    uint32_t totalFiles = 0;                 // 总文件数
    uint64_t bytesProcessed = 0;             // 已处理字节数
    uint64_t totalBytes = 0;                 // 总字节数
    uint8_t percentage = 0;                  // 进度百分比（0-100）

    /**
     * 获取阶段名称
     */
    std::string getPhaseName() const {
        switch (phase) {
            case BackupPhase::IDLE: return "空闲";
            case BackupPhase::SCANNING: return "扫描文件";
            case BackupPhase::FILTERING: return "过滤文件";
            case BackupPhase::COMPRESSING: return "压缩数据";
            case BackupPhase::ENCRYPTING: return "加密数据";
            case BackupPhase::DECRYPTING: return "解密数据";
            case BackupPhase::DECOMPRESSING: return "解压数据";
            case BackupPhase::PACKING: return "打包文件";
            case BackupPhase::VERIFYING: return "验证备份";
            case BackupPhase::COMPLETED: return "完成";
            case BackupPhase::FAILED: return "失败";
            case BackupPhase::CANCELLED: return "已取消";
            default: return "未知";
        }
    }
};

/**
 * 备份结果类
 * 记录备份任务的最终结果
 */
struct BackupResult {
    bool success = false;                // 是否成功
    uint32_t filesBackedup = 0;          // 备份的文件数
    uint32_t filesSkipped = 0;           // 跳过的文件数
    uint64_t totalSize = 0;              // 原始总大小
    uint64_t compressedSize = 0;         // 压缩后大小
    uint64_t backupSize = 0;             // 最终备份文件大小
    uint64_t duration = 0;               // 耗时（毫秒）
    std::string errorMessage;            // 错误信息
    std::string backupFilePath;          // 备份文件路径

    /**
     * 计算压缩率
     */
    double getCompressionRatio() const {
        if (totalSize == 0) return 0.0;
        return 100.0 * (1.0 - static_cast<double>(compressedSize) / static_cast<double>(totalSize));
    }

    /**
     * 计算空间节省率
     */
    double getSpaceSaving() const {
        if (totalSize == 0) return 0.0;
        return 100.0 * (1.0 - static_cast<double>(backupSize) / static_cast<double>(totalSize));
    }
};

/**
 * 还原结果类
 */
struct RestoreResult {
    bool success = false;                // 是否成功
    uint32_t filesRestored = 0;          // 还原的文件数
    uint32_t filesFailed = 0;            // 还原失败的文件数
    uint64_t totalSize = 0;              // 还原总大小
    uint64_t duration = 0;               // 耗时（毫秒）
    std::string errorMessage;            // 错误信息
};

} // namespace DataBackup