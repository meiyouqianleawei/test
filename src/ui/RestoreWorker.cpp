#include "RestoreWorker.h"
#include "encryption/XOREncryption.h"
#include "core/MetadataManager.h"
#include <filesystem>
#include <fstream>
#include <chrono>
#include <windows.h>

namespace fs = std::filesystem;

// UTF-8 <-> wide helpers（与 RealtimeBackupMonitor 里保持一致的语义）
static std::wstring utf8ToWideR(const std::string& s) {
    if (s.empty()) return L"";
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), static_cast<int>(s.size()), nullptr, 0);
    if (len <= 0) return L"";
    std::wstring w(len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), static_cast<int>(s.size()), &w[0], len);
    return w;
}
static fs::path pathFromUtf8R(const std::string& s) { return fs::path(utf8ToWideR(s)); }

namespace DataBackup {

RestoreWorker::RestoreWorker(QObject* parent)
    : QThread(parent)
    , restoreEngine_(std::make_unique<RestoreEngine>())
    , cancelled_(false)
{
}

RestoreWorker::~RestoreWorker() {
    if (isRunning()) {
        cancel();
        wait();
    }
}

void RestoreWorker::setBackupFile(const std::string& filePath) {
    backupFilePath_ = filePath;
}

void RestoreWorker::setTargetDirectory(const std::string& directory) {
    targetDirectory_ = directory;
}

void RestoreWorker::setPassword(const std::string& password) {
    password_ = password;
}

void RestoreWorker::setMode(Mode mode) {
    mode_ = mode;
}

void RestoreWorker::cancel() {
    cancelled_ = true;
    if (restoreEngine_) {
        restoreEngine_->cancelRestore();
    }
}

bool RestoreWorker::isRunning() const {
    return QThread::isRunning();
}

void RestoreWorker::run() {
    cancelled_ = false;

    if (mode_ == Mode::Folder) {
        RestoreResult result = restoreFolder();
        emit restoreComplete(result);
        return;
    }

    // Package 模式：走 RestoreEngine
    restoreEngine_->setProgressCallback([this](const BackupProgress& progress) {
        onProgress(progress);
    });
    RestoreResult result = restoreEngine_->restore(backupFilePath_, targetDirectory_, password_);
    emit restoreComplete(result);
}

void RestoreWorker::onProgress(const BackupProgress& progress) {
    emit progressUpdate(progress);
}

RestoreResult RestoreWorker::restoreFolder() {
    RestoreResult result;
    auto startTime = std::chrono::high_resolution_clock::now();

    fs::path srcRoot = pathFromUtf8R(backupFilePath_);
    fs::path tgtRoot = pathFromUtf8R(targetDirectory_);

    if (!fs::exists(srcRoot) || !fs::is_directory(srcRoot)) {
        result.success = false;
        result.errorMessage = "备份目录不存在";
        return result;
    }

    // 目标目录准备
    std::error_code ec;
    fs::create_directories(tgtRoot, ec);
    if (ec) {
        result.success = false;
        result.errorMessage = "无法创建还原目录: " + ec.message();
        return result;
    }

    bool needDecrypt = !password_.empty();
    XOREncryption decryptor;

    // 先统计需处理的文件数（跳过 .meta）
    uint32_t total = 0;
    for (auto it = fs::recursive_directory_iterator(srcRoot,
             fs::directory_options::skip_permission_denied);
         it != fs::recursive_directory_iterator(); ++it) {
        if (cancelled_) break;
        if (it->is_directory()) continue;
        if (it->path().extension() == L".meta") continue;
        total++;
    }

    // 发出初始进度
    {
        BackupProgress p;
        p.phase = needDecrypt ? BackupPhase::DECRYPTING : BackupPhase::PACKING;
        p.totalFiles = total;
        p.percentage = 0;
        emit progressUpdate(p);
    }

    MetadataManager metaMgr;

    uint32_t done = 0;
    for (auto it = fs::recursive_directory_iterator(srcRoot,
             fs::directory_options::skip_permission_denied);
         it != fs::recursive_directory_iterator(); ++it) {
        if (cancelled_) {
            result.success = false;
            result.errorMessage = "已取消";
            return result;
        }
        if (it->is_directory()) continue;
        const fs::path& src = it->path();
        if (src.extension() == L".meta") continue;

        fs::path rel = fs::relative(src, srcRoot, ec);
        if (ec) { rel = src.filename(); ec.clear(); }
        fs::path dst = tgtRoot / rel;

        // 建父目录
        fs::create_directories(dst.parent_path(), ec);
        ec.clear();

        // 读取源
        std::ifstream in(src, std::ios::binary);
        if (!in) {
            result.filesFailed++;
            done++;
            continue;
        }
        std::vector<uint8_t> data((std::istreambuf_iterator<char>(in)),
                                  std::istreambuf_iterator<char>());
        in.close();

        // 解密
        std::vector<uint8_t> plain;
        if (needDecrypt) {
            if (!decryptor.decrypt(data, plain, password_)) {
                result.filesFailed++;
                done++;
                // 上报进度
                BackupProgress p;
                p.phase = BackupPhase::DECRYPTING;
                p.filesProcessed = done;
                p.totalFiles = total;
                p.percentage = total ? static_cast<uint8_t>(done * 100 / total) : 100;
                p.currentFile = src.filename().string();
                emit progressUpdate(p);
                continue;
            }
        } else {
            plain = std::move(data);
        }

        // 目标写入前清理只读位
        if (fs::exists(dst)) {
            DWORD attrs = GetFileAttributesW(dst.wstring().c_str());
            if (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_READONLY)) {
                SetFileAttributesW(dst.wstring().c_str(), attrs & ~FILE_ATTRIBUTE_READONLY);
            }
        }
        std::ofstream out(dst, std::ios::binary | std::ios::trunc);
        if (!out) {
            result.filesFailed++;
            done++;
            continue;
        }
        out.write(reinterpret_cast<const char*>(plain.data()),
                  static_cast<std::streamsize>(plain.size()));
        out.close();

        // 若有 .meta,尝试恢复时间戳/属性
        fs::path metaPath = src;
        metaPath += L".meta";
        if (fs::exists(metaPath)) {
            FileInfo info = metaMgr.loadMetadataFromFile(metaPath);
            if (!info.path.empty()) {
                info.path = dst;
                metaMgr.setMetadata(info, dst);
            }
        }

        result.filesRestored++;
        result.totalSize += plain.size();
        done++;

        BackupProgress p;
        p.phase = needDecrypt ? BackupPhase::DECRYPTING : BackupPhase::PACKING;
        p.filesProcessed = done;
        p.totalFiles = total;
        p.percentage = total ? static_cast<uint8_t>(done * 100 / total) : 100;
        p.currentFile = src.filename().string();
        emit progressUpdate(p);
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        endTime - startTime).count();
    result.success = (result.filesFailed == 0)
                     || (result.filesRestored > 0 && result.filesFailed < result.filesRestored);
    if (!result.success) {
        result.errorMessage = "全部文件解密失败,密码可能错误";
    }

    // 完成事件
    BackupProgress finalP;
    finalP.phase = BackupPhase::COMPLETED;
    finalP.filesProcessed = done;
    finalP.totalFiles = total;
    finalP.percentage = 100;
    emit progressUpdate(finalP);

    return result;
}

} // namespace DataBackup
