#include "BackupEngine.h"
#include "compression/HuffmanCompression.h"
#include "compression/ZlibCompression.h"
#include "encryption/XOREncryption.h"
#include "core/MetadataManager.h"
#include <filesystem>
#include <chrono>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;

namespace DataBackup {

BackupEngine::BackupEngine()
    : cancelled_(false)
    , running_(false)
    , scanner_(std::make_unique<FileScanner>())
    , packager_(std::make_unique<Packager>()) {
}

BackupEngine::~BackupEngine() {
    cancelBackup();
}

BackupResult BackupEngine::createBackup(const BackupConfig& config,
                                        ProgressCallback progressCallback) {
    BackupResult result;

    // 检查是否已在运行
    if (running_) {
        result.success = false;
        result.errorMessage = "Backup already in progress";
        return result;
    }

    // 重置状态
    config_ = config;
    cancelled_ = false;
    running_ = true;
    progressCallback_ = progressCallback;

    auto startTime = std::chrono::high_resolution_clock::now();

    try {
        // 阶段1：扫描文件
        updateProgress(BackupPhase::SCANNING, 0, 0);
        if (!scanFiles(config)) {
            result.success = false;
            result.errorMessage = "Failed to scan files";
            running_ = false;
            return result;
        }

        if (cancelled_) {
            result.success = false;
            result.errorMessage = "Backup cancelled during scanning";
            running_ = false;
            return result;
        }

        // 阶段2：过滤文件
        updateProgress(BackupPhase::FILTERING, 0, scannedFiles_.size());
        filterFiles();

        if (cancelled_) {
            result.success = false;
            result.errorMessage = "Backup cancelled during filtering";
            running_ = false;
            return result;
        }

        // 阶段3：压缩和加密
        updateProgress(BackupPhase::COMPRESSING, 0, filteredFiles_.size());
        if (!compressAndEncrypt()) {
            result.success = false;
            result.errorMessage = "Failed to compress/encrypt files";
            running_ = false;
            return result;
        }

        if (cancelled_) {
            result.success = false;
            result.errorMessage = "Backup cancelled during compression";
            running_ = false;
            return result;
        }

        // 阶段4：打包文件
        updateProgress(BackupPhase::PACKING, 0, filesToBackup_.size());
        if (!packFiles(config)) {
            result.success = false;
            result.errorMessage = "Failed to pack files";
            running_ = false;
            return result;
        }

        if (cancelled_) {
            result.success = false;
            result.errorMessage = "Backup cancelled during packing";
            running_ = false;
            return result;
        }

        // 阶段5：验证备份
        if (config.verifyAfterBackup) {
            updateProgress(BackupPhase::VERIFYING, 0, filesToBackup_.size());
            if (!verifyBackup(config)) {
                result.success = false;
                result.errorMessage = "Backup verification failed";
                running_ = false;
                return result;
            }
        }

        // 完成
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

        result.success = true;
        result.filesBackedup = static_cast<uint32_t>(filesToBackup_.size());
        result.filesSkipped = static_cast<uint32_t>(scannedFiles_.size() - filteredFiles_.size());
        result.backupFilePath = config.targetPath;
        result.duration = duration.count();

        // 计算总大小(原始)
        for (const auto& file : filesToBackup_) {
            if (!file.isDirectory) {
                result.totalSize += file.size;
            }
        }

        // 读取包文件的实际字节数,回填 backupSize/compressedSize
        try {
            if (fs::exists(config.targetPath)) {
                result.backupSize = fs::file_size(config.targetPath);
                result.compressedSize = result.backupSize;
            }
        } catch (...) {
            // 忽略,保留默认 0
        }

        updateProgress(BackupPhase::COMPLETED, result.filesBackedup, result.filesBackedup);

    } catch (const std::exception& e) {
        result.success = false;
        result.errorMessage = "Exception: " + std::string(e.what());
        updateProgress(BackupPhase::FAILED, 0, 0);
    }

    running_ = false;
    return result;
}

bool BackupEngine::scanFiles(const BackupConfig& config) {
    try {
        // 设置扫描选项
        scanner_->setFollowSymlinks(config.followSymlinks);
        // 注意：skipHardlinks选项已在FileScanner中默认处理

        std::vector<FileInfo> files;
        bool success = scanner_->scan(config.sourcePath, files);

        if (success) {
            // 若目标包文件位于源目录内(或就是源目录中之前一次备份的产物),
            // 从扫描列表中排除,避免第二次备份时把自身当作输入而打开失败。
            std::error_code ec;
            fs::path targetAbs = fs::weakly_canonical(fs::path(config.targetPath), ec);
            if (ec) targetAbs = fs::absolute(fs::path(config.targetPath));

            scannedFiles_.clear();
            scannedFiles_.reserve(files.size());
            for (const auto& f : files) {
                std::error_code ec2;
                fs::path abs = fs::weakly_canonical(f.path, ec2);
                if (ec2) abs = fs::absolute(f.path);
                if (abs == targetAbs) {
                    continue;
                }
                scannedFiles_.push_back(f);
            }
        }

        return success;
    } catch (const std::exception& e) {
        reportError("Scan failed: " + std::string(e.what()));
        return false;
    }
}

bool BackupEngine::filterFiles() {
    filteredFiles_.clear();

    if (config_.filters.empty()) {
        // 没有过滤器，接受所有文件
        filteredFiles_ = scannedFiles_;
        return true;
    }

    FilterChain chain(config_.filterModeAnd ? FilterChain::Mode::AND
                                             : FilterChain::Mode::OR);
    for (const auto& filter : config_.filters) {
        chain.addFilter(filter);
    }

    for (const auto& file : scannedFiles_) {
        if (chain.accept(file)) {
            filteredFiles_.push_back(file);
        }
    }

    return true;
}

bool BackupEngine::compressAndEncrypt() {
    filesToBackup_.clear();
    encryptor_.reset();
    compressor_.reset();

    // 压缩策略保留到 packFiles 阶段传给 Packager
    if (config_.compressionAlgorithm == "huffman") {
        compressor_ = std::make_unique<HuffmanCompression>();
    } else if (config_.compressionAlgorithm == "zlib") {
        compressor_ = std::make_unique<ZlibCompression>(config_.compressionLevel);
    }

    // 加密策略保留到 packFiles 阶段传给 Packager
    if (config_.encryptionAlgorithm == "xor" && !config_.password.empty()) {
        encryptor_ = std::make_unique<XOREncryption>();
    }

    filesToBackup_ = filteredFiles_;
    return true;
}

bool BackupEngine::packFiles(const BackupConfig& config) {
    try {
        // 提取元数据
        MetadataManager metaManager;
        std::vector<FileInfo> filesWithMetadata;

        uint32_t processedFiles = 0;
        for (const auto& file : filesToBackup_) {
            if (cancelled_) {
                return false;
            }

            FileInfo fileWithMeta = file;
            if (!file.isDirectory) {
                fileWithMeta = metaManager.extractMetadata(file.path);
            }
            filesWithMetadata.push_back(fileWithMeta);

            processedFiles++;
            updateProgress(BackupPhase::PACKING, processedFiles, filesToBackup_.size());
        }

        // 打包文件(若启用加密/压缩则由 Packager 处理)
        bool success = packager_->pack(filesWithMetadata, config.targetPath,
            [this](int current, int total) {
                updateProgress(BackupPhase::PACKING, current, total);
            },
            config.preserveDirectoryStructure ? config.sourcePath : "",
            encryptor_.get(),
            config.password,
            compressor_.get());

        return success;

    } catch (const std::exception& e) {
        reportError("Pack failed: " + std::string(e.what()));
        return false;
    }
}

bool BackupEngine::verifyBackup(const BackupConfig& config) {
    try {
        return packager_->verifyPackage(config.targetPath);
    } catch (const std::exception& e) {
        reportError("Verification failed: " + std::string(e.what()));
        return false;
    }
}

bool BackupEngine::cancelBackup() {
    if (!running_) {
        return false;
    }

    cancelled_ = true;
    updateProgress(BackupPhase::CANCELLED, 0, 0);
    return true;
}

BackupProgress BackupEngine::getProgress() const {
    std::lock_guard<std::mutex> lock(progressMutex_);
    return progress_;
}

void BackupEngine::setProgressCallback(ProgressCallback callback) {
    std::lock_guard<std::mutex> lock(progressMutex_);
    progressCallback_ = callback;
}

bool BackupEngine::isRunning() const {
    return running_;
}

void BackupEngine::updateProgress(BackupPhase phase, uint32_t filesProcessed, uint32_t totalFiles) {
    // 更新进度（加锁）
    ProgressCallback callback;
    {
        std::lock_guard<std::mutex> lock(progressMutex_);
        progress_.phase = phase;
        progress_.filesProcessed = filesProcessed;
        progress_.totalFiles = totalFiles;

        if (totalFiles > 0) {
            progress_.percentage = static_cast<uint8_t>(100 * filesProcessed / totalFiles);
        }

        callback = progressCallback_;  // 复制回调
    }
    // 释放锁

    // 调用回调（不持有锁，避免死锁）
    if (callback) {
        callback(progress_);
    }
}

void BackupEngine::reportError(const std::string& message) {
    std::cerr << "[BackupEngine Error] " << message << std::endl;
}

} // namespace DataBackup