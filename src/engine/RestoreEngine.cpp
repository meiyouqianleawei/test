#include "RestoreEngine.h"
#include "compression/HuffmanCompression.h"
#include "compression/ZlibCompression.h"
#include "encryption/XOREncryption.h"
#include "core/MetadataManager.h"
#include <filesystem>
#include <chrono>
#include <iostream>

namespace fs = std::filesystem;

namespace DataBackup {

RestoreEngine::RestoreEngine()
    : cancelled_(false)
    , running_(false)
    , packager_(std::make_unique<Packager>()) {
}

RestoreEngine::~RestoreEngine() {
    cancelRestore();
}

RestoreResult RestoreEngine::restore(const std::string& backupFilePath,
                                     const std::string& targetDirectory,
                                     const std::string& password,
                                     ProgressCallback progressCallback) {
    RestoreResult result;

    // 检查是否已在运行
    if (running_) {
        result.success = false;
        result.errorMessage = "Restore already in progress";
        return result;
    }

    // 重置状态
    cancelled_ = false;
    running_ = true;
    progressCallback_ = progressCallback;

    auto startTime = std::chrono::high_resolution_clock::now();

    try {
        // 阶段1:解包+解密(由 Packager 内部完成)
        updateProgress(BackupPhase::PACKING, 0, 0);
        int errCode = 0;
        bool unpackOk = packager_->unpack(backupFilePath, targetDirectory,
            [this](int current, int total) {
                updateProgress(BackupPhase::PACKING, current, total);
            },
            password, &errCode);

        if (!unpackOk) {
            result.success = false;
            if (errCode == 1) {
                result.errorMessage = password.empty()
                    ? "备份文件已加密,请输入正确的解密密码"
                    : "解密密码错误";
                updateProgress(BackupPhase::FAILED, 0, 0);
            } else {
                result.errorMessage = "Failed to unpack backup";
                updateProgress(BackupPhase::FAILED, 0, 0);
            }
            running_ = false;
            return result;
        }

        if (cancelled_) {
            result.success = false;
            result.errorMessage = "Restore cancelled during unpacking";
            running_ = false;
            return result;
        }

        // 阶段2:标记解密阶段完成(实际解密已在 unpack 中完成)
        updateProgress(BackupPhase::DECRYPTING, 0, 0);
        if (!decryptAndDecompress(password)) {
            result.success = false;
            result.errorMessage = "Failed to decrypt/decompress";
            running_ = false;
            return result;
        }

        if (cancelled_) {
            result.success = false;
            result.errorMessage = "Restore cancelled during decryption";
            running_ = false;
            return result;
        }

        // 阶段3：恢复元数据
        updateProgress(BackupPhase::VERIFYING, 0, 0);
        if (!restoreMetadata()) {
            result.success = false;
            result.errorMessage = "Failed to restore metadata";
            running_ = false;
            return result;
        }

        // 完成:回填还原后的文件数量和原始总大小(来自包索引)
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

        try {
            PackageInfo pkg = packager_->getPackageInfo(backupFilePath);
            result.filesRestored = pkg.fileCount;
            result.totalSize = pkg.totalSize;
        } catch (...) {
            // 保留默认 0
        }

        result.success = true;
        result.duration = duration.count();

        updateProgress(BackupPhase::COMPLETED, 0, 0);

    } catch (const std::exception& e) {
        result.success = false;
        result.errorMessage = "Exception: " + std::string(e.what());
        updateProgress(BackupPhase::FAILED, 0, 0);
    }

    running_ = false;
    return result;
}

bool RestoreEngine::verifyBackup(const std::string& backupFilePath) {
    try {
        return packager_->verifyPackage(backupFilePath);
    } catch (const std::exception& e) {
        std::cerr << "[RestoreEngine Error] Verification failed: " << e.what() << std::endl;
        return false;
    }
}

std::string RestoreEngine::getBackupInfo(const std::string& backupFilePath) {
    try {
        PackageInfo info = packager_->getPackageInfo(backupFilePath);
        // 转换为JSON格式（简化版本）
        std::string json = "{";
        json += "\"version\":\"" + info.version + "\",";
        json += "\"fileCount\":" + std::to_string(info.fileCount) + ",";
        json += "\"totalSize\":" + std::to_string(info.totalSize);
        json += "}";
        return json;
    } catch (const std::exception& e) {
        return "{}";
    }
}

bool RestoreEngine::cancelRestore() {
    if (!running_) {
        return false;
    }

    cancelled_ = true;
    updateProgress(BackupPhase::CANCELLED, 0, 0);
    return true;
}

BackupProgress RestoreEngine::getProgress() const {
    return progress_;
}

void RestoreEngine::setProgressCallback(ProgressCallback callback) {
    progressCallback_ = callback;
}

bool RestoreEngine::isRunning() const {
    return running_;
}

bool RestoreEngine::decryptAndDecompress(const std::string& password) {
    // 简化版本：解包时已经还原了原始文件
    // 实际应用中，这里需要解密和解压临时文件
    // 然后替换解包后的文件

    // TODO: 实现解密和解压逻辑
    // 1. 遍历解包后的文件
    // 2. 如果加密了，解密文件
    // 3. 如果压缩了，解压文件
    // 4. 替换原始文件

    return true;
}

bool RestoreEngine::restoreMetadata() {
    // 简化版本：元数据已经在解包时恢复
    // 实际应用中，这里需要从备份文件中读取元数据并应用到文件

    // TODO: 实现元数据恢复逻辑
    // 1. 读取备份文件中的元数据
    // 2. 应用到还原的文件

    return true;
}

void RestoreEngine::updateProgress(BackupPhase phase, uint32_t filesProcessed, uint32_t totalFiles) {
    progress_.phase = phase;
    progress_.filesProcessed = filesProcessed;
    progress_.totalFiles = totalFiles;

    if (totalFiles > 0) {
        progress_.percentage = static_cast<uint8_t>(100 * filesProcessed / totalFiles);
    }

    if (progressCallback_) {
        progressCallback_(progress_);
    }
}

void RestoreEngine::reportError(const std::string& message) {
    std::cerr << "[RestoreEngine Error] " << message << std::endl;
}

} // namespace DataBackup