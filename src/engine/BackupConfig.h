#pragma once
#include <string>
#include <vector>
#include <memory>
#include "filter/IFileFilter.h"
#include "compression/ICompressionStrategy.h"
#include "encryption/IEncryptionStrategy.h"

namespace DataBackup {

/**
 * 备份配置类
 * 定义备份任务的所有配置参数
 */
struct BackupConfig {
    // 路径配置
    std::string sourcePath;              // 源目录路径
    std::string targetPath;              // 目标备份文件路径
    std::string tempPath;                // 临时文件路径（可选）

    // 压缩配置
    std::string compressionAlgorithm;    // 压缩算法：none, huffman, zlib
    int compressionLevel = -1;           // 压缩级别（仅zlib使用）

    // 加密配置
    std::string encryptionAlgorithm;     // 加密算法：none, xor
    std::string password;                // 加密密码

    // 过滤器配置
    std::vector<std::shared_ptr<IFileFilter>> filters;  // 过滤器链
    // 过滤器组合模式:true=AND(默认),false=OR
    bool filterModeAnd = true;

    // 其他配置
    bool preserveDirectoryStructure = true;  // 保留目录结构
    bool verifyAfterBackup = true;           // 备份后验证
    bool skipHardlinks = true;               // 跳过硬链接
    bool followSymlinks = false;             // 跟随符号链接

    /**
     * 默认构造函数
     */
    BackupConfig() = default;

    /**
     * 简化构造函数
     */
    BackupConfig(const std::string& source, const std::string& target)
        : sourcePath(source), targetPath(target) {}
};

} // namespace DataBackup