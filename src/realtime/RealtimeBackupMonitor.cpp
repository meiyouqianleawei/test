#include "RealtimeBackupMonitor.h"
#include "encryption/XOREncryption.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <chrono>

namespace fs = std::filesystem;

// 辅助函数：宽字符转窄字符（支持中文路径）
static std::string wideToUtf8(const std::wstring& wideStr) {
    if (wideStr.empty()) return "";
    int len = WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(),
                                  static_cast<int>(wideStr.size()),
                                  nullptr, 0, nullptr, nullptr);
    if (len <= 0) return "";
    std::string utf8Str(len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(),
                        static_cast<int>(wideStr.size()),
                        &utf8Str[0], len, nullptr, nullptr);
    return utf8Str;
}

// 辅助函数：窄字符转宽字符
static std::wstring utf8ToWide(const std::string& utf8Str) {
    if (utf8Str.empty()) return L"";
    int len = MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(),
                                  static_cast<int>(utf8Str.size()),
                                  nullptr, 0);
    if (len <= 0) return L"";
    std::wstring wideStr(len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(),
                        static_cast<int>(utf8Str.size()),
                        &wideStr[0], len);
    return wideStr;
}

// 从 UTF-8 字符串构造 fs::path (Windows 下 fs::path(std::string) 按系统代码页解释,
// 中文路径会被误解析,必须先转成宽字符)
static std::filesystem::path pathFromUtf8(const std::string& utf8Str) {
    return std::filesystem::path(utf8ToWide(utf8Str));
}

// 把 fs::path 转回 UTF-8 std::string
static std::string pathToUtf8(const std::filesystem::path& p) {
    return wideToUtf8(p.wstring());
}

namespace DataBackup {

RealtimeBackupMonitor::RealtimeBackupMonitor()
    : status_(RealtimeBackupStatus::IDLE)
    , running_(false)
    , paused_(false)
    , stopRequested_(false)
    , directoryHandle_(INVALID_HANDLE_VALUE)
    , stopEvent_(nullptr)
    , metadataManager_(std::make_unique<MetadataManager>()) {
}

RealtimeBackupMonitor::~RealtimeBackupMonitor() {
    stopMonitor();
}

bool RealtimeBackupMonitor::startMonitor(const RealtimeBackupConfig& config,
                                         FileChangeCallback callback) {
    // 检查是否已在运行
    if (isRunning()) {
        return false;
    }

    // 验证配置
    if (config.sourcePath.empty() || config.targetPath.empty()) {
        return false;
    }

    // 检查源目录是否存在
    fs::path srcPath = pathFromUtf8(config.sourcePath);
    fs::path tgtPath = pathFromUtf8(config.targetPath);
    if (!fs::exists(srcPath) || !fs::is_directory(srcPath)) {
        return false;
    }

    // 目标目录不能等于源目录（会导致自反循环）
    std::error_code ec;
    fs::path srcAbs = fs::weakly_canonical(srcPath, ec);
    if (ec) srcAbs = fs::absolute(srcPath);
    fs::path tgtAbs = fs::weakly_canonical(tgtPath, ec);
    if (ec) tgtAbs = fs::absolute(tgtPath);
    if (srcAbs == tgtAbs) {
        return false;
    }

    // 创建目标目录（如果不存在）
    try {
        fs::create_directories(tgtPath);
    } catch (const std::exception& e) {
        std::cerr << "[RealtimeBackupMonitor] Failed to create target directory: " << e.what() << std::endl;
        return false;
    }

    // 设置配置
    {
        std::lock_guard<std::mutex> lock(configMutex_);
        config_ = config;
    }

    // 设置回调
    {
        std::lock_guard<std::mutex> lock(callbackMutex_);
        callback_ = callback;
    }

    // 创建加密器（如果启用）
    if (config.enableEncryption && !config.encryptionPassword.empty()) {
        encryptor_ = std::make_unique<XOREncryption>();
    } else {
        encryptor_.reset();
    }

    // 创建停止事件
    stopEvent_ = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    if (!stopEvent_) {
        std::cerr << "[RealtimeBackupMonitor] Failed to create stop event" << std::endl;
        return false;
    }

    // 重置状态
    stopRequested_ = false;
    paused_ = false;
    running_ = true;
    pendingRenameOldPath_.clear();

    // 更新状态
    {
        std::lock_guard<std::mutex> lock(statusMutex_);
        status_ = RealtimeBackupStatus::RUNNING;
    }

    // 更新统计
    {
        std::lock_guard<std::mutex> lock(statsMutex_);
        stats_ = RealtimeBackupStats();
        stats_.startTime = getCurrentTimestamp();
    }

    // 启动监控线程
    monitorThread_ = std::thread(&RealtimeBackupMonitor::monitorThread, this);

    return true;
}

void RealtimeBackupMonitor::stopMonitor() {
    if (!isRunning()) {
        return;
    }

    // 请求停止
    stopRequested_ = true;
    running_ = false;

    // 触发停止事件
    if (stopEvent_) {
        SetEvent(stopEvent_);
    }

    // 等待线程结束
    if (monitorThread_.joinable()) {
        monitorThread_.join();
    }

    // 关闭句柄
    if (directoryHandle_ != INVALID_HANDLE_VALUE) {
        CloseHandle(directoryHandle_);
        directoryHandle_ = INVALID_HANDLE_VALUE;
    }

    if (stopEvent_) {
        CloseHandle(stopEvent_);
        stopEvent_ = nullptr;
    }

    // 更新状态
    {
        std::lock_guard<std::mutex> lock(statusMutex_);
        status_ = RealtimeBackupStatus::IDLE;
    }
}

void RealtimeBackupMonitor::pauseMonitor() {
    if (!isRunning()) {
        return;
    }

    paused_ = true;

    {
        std::lock_guard<std::mutex> lock(statusMutex_);
        status_ = RealtimeBackupStatus::PAUSED;
    }
}

void RealtimeBackupMonitor::resumeMonitor() {
    if (!isRunning()) {
        return;
    }

    paused_ = false;

    {
        std::lock_guard<std::mutex> lock(statusMutex_);
        status_ = RealtimeBackupStatus::RUNNING;
    }
}

RealtimeBackupStatus RealtimeBackupMonitor::getStatus() const {
    std::lock_guard<std::mutex> lock(statusMutex_);
    return status_;
}

RealtimeBackupStats RealtimeBackupMonitor::getStats() const {
    std::lock_guard<std::mutex> lock(statsMutex_);
    return stats_;
}

std::string RealtimeBackupMonitor::getMonitoredPath() const {
    std::lock_guard<std::mutex> lock(configMutex_);
    return config_.sourcePath;
}

bool RealtimeBackupMonitor::isRunning() const {
    return running_;
}

bool RealtimeBackupMonitor::backupFileNow(const std::string& filePath) {
    // 检查文件是否存在
    if (!fs::exists(pathFromUtf8(filePath))) {
        return false;
    }

    // 备份文件
    return backupFile(filePath);
}

void RealtimeBackupMonitor::monitorThread() {
    // 读取配置副本
    std::string sourcePath;
    bool includeSubdirs;
    bool doInitialSync;
    {
        std::lock_guard<std::mutex> lock(configMutex_);
        sourcePath = config_.sourcePath;
        includeSubdirs = config_.includeSubdirectories;
        doInitialSync = config_.initialFullSync;
    }

    // 打开目录句柄
    std::wstring widePath = utf8ToWide(sourcePath);
    directoryHandle_ = CreateFileW(
        widePath.c_str(),
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
        nullptr
    );

    if (directoryHandle_ == INVALID_HANDLE_VALUE) {
        std::cerr << "[RealtimeBackupMonitor] Failed to open directory handle" << std::endl;
        std::lock_guard<std::mutex> lock(statusMutex_);
        status_ = RealtimeBackupStatus::ERROR_STATE;
        return;
    }

    // 启动前先做全量同步
    if (doInitialSync) {
        performInitialSync();
    }

    // 准备异步IO
    OVERLAPPED overlapped = { 0 };
    overlapped.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);

    // 监控缓冲区（增大以容纳批量事件）
    constexpr DWORD bufferSize = 64 * 1024;
    std::vector<BYTE> buffer(bufferSize);
    DWORD bytesReturned = 0;

    // 监控的文件变化类型
    const DWORD notifyFilter = FILE_NOTIFY_CHANGE_FILE_NAME |
                               FILE_NOTIFY_CHANGE_DIR_NAME |
                               FILE_NOTIFY_CHANGE_ATTRIBUTES |
                               FILE_NOTIFY_CHANGE_SIZE |
                               FILE_NOTIFY_CHANGE_LAST_WRITE |
                               FILE_NOTIFY_CHANGE_CREATION;

    // 主监控循环
    while (running_ && !stopRequested_) {
        // 暂停时等待恢复
        if (paused_) {
            // 使用 stop 事件轻量等待
            WaitForSingleObject(stopEvent_, 200);
            continue;
        }

        ResetEvent(overlapped.hEvent);

        BOOL success = ReadDirectoryChangesW(
            directoryHandle_,
            buffer.data(),
            bufferSize,
            includeSubdirs ? TRUE : FALSE,
            notifyFilter,
            &bytesReturned,
            &overlapped,
            nullptr
        );

        if (!success) {
            std::cerr << "[RealtimeBackupMonitor] ReadDirectoryChangesW failed" << std::endl;
            break;
        }

        // 等待变化或停止
        HANDLE events[2] = { stopEvent_, overlapped.hEvent };
        DWORD waitResult = WaitForMultipleObjects(2, events, FALSE, INFINITE);

        if (waitResult == WAIT_OBJECT_0) {
            // 停止事件触发，取消 IO 后退出
            CancelIoEx(directoryHandle_, &overlapped);
            GetOverlappedResult(directoryHandle_, &overlapped, &bytesReturned, TRUE);
            break;
        }

        // 获取结果
        success = GetOverlappedResult(directoryHandle_, &overlapped, &bytesReturned, FALSE);
        if (!success || bytesReturned == 0) {
            continue;
        }

        // 处理文件变化通知
        BYTE* ptr = buffer.data();
        while (true) {
            FILE_NOTIFY_INFORMATION* fni = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(ptr);

            std::wstring fileName(fni->FileName, fni->FileNameLength / sizeof(wchar_t));

            // 用宽字符组装绝对路径,再转回 UTF-8 传给下游(下游会用 pathFromUtf8 还原)
            fs::path fullPath = pathFromUtf8(sourcePath) / fileName;
            std::string filePath = pathToUtf8(fullPath);

            handleFileChange(filePath, fni->Action);

            if (fni->NextEntryOffset == 0) break;
            ptr += fni->NextEntryOffset;
        }
    }

    // 清理
    if (overlapped.hEvent) {
        CloseHandle(overlapped.hEvent);
    }
}

void RealtimeBackupMonitor::performInitialSync() {
    std::string sourcePath;
    bool includeSubdirs;
    {
        std::lock_guard<std::mutex> lock(configMutex_);
        sourcePath = config_.sourcePath;
        includeSubdirs = config_.includeSubdirectories;
    }

    fs::path srcRoot = pathFromUtf8(sourcePath);

    try {
        if (includeSubdirs) {
            for (auto it = fs::recursive_directory_iterator(srcRoot,
                    fs::directory_options::skip_permission_denied);
                 it != fs::recursive_directory_iterator(); ++it) {
                if (stopRequested_) return;
                if (it->is_directory()) continue;
                std::string p = pathToUtf8(it->path());
                if (isInsideTarget(p)) continue;
                if (!shouldBackupFile(p)) continue;
                backupFile(p);
            }
        } else {
            for (auto& entry : fs::directory_iterator(srcRoot,
                    fs::directory_options::skip_permission_denied)) {
                if (stopRequested_) return;
                if (entry.is_directory()) continue;
                std::string p = pathToUtf8(entry.path());
                if (isInsideTarget(p)) continue;
                if (!shouldBackupFile(p)) continue;
                backupFile(p);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "[RealtimeBackupMonitor] Initial sync error: " << e.what() << std::endl;
    }
}

void RealtimeBackupMonitor::handleFileChange(const std::string& filePath, DWORD action) {
    // 若目标位于源内,排除掉写入目标目录时产生的自反通知
    if (isInsideTarget(filePath)) {
        return;
    }

    // 获取变化类型
    FileChangeType changeType;
    switch (action) {
        case FILE_ACTION_ADDED:   changeType = FileChangeType::CREATED;  break;
        case FILE_ACTION_REMOVED: changeType = FileChangeType::DELETED;  break;
        case FILE_ACTION_MODIFIED:changeType = FileChangeType::MODIFIED; break;
        case FILE_ACTION_RENAMED_OLD_NAME:
        case FILE_ACTION_RENAMED_NEW_NAME:
            changeType = FileChangeType::RENAMED;
            break;
        default: return;
    }

    // 通知回调（不持锁）
    FileChangeCallback callback;
    {
        std::lock_guard<std::mutex> lock(callbackMutex_);
        callback = callback_;
    }
    if (callback) {
        FileChangeEvent event;
        event.type = changeType;
        event.filePath = filePath;
        if (action == FILE_ACTION_RENAMED_NEW_NAME) {
            event.oldFilePath = pendingRenameOldPath_;
        }
        event.timestamp = getCurrentTimestamp();
        callback(event);
    }

    // 若变化文件不匹配过滤条件（且不是删除/重命名事件），跳过
    // 删除/重命名 OLD_NAME 阶段没法确定文件类型（因为文件已不在），所以放行
    bool skipFilter = (action == FILE_ACTION_REMOVED)
                   || (action == FILE_ACTION_RENAMED_OLD_NAME)
                   || (action == FILE_ACTION_RENAMED_NEW_NAME);
    if (!skipFilter && !shouldBackupFile(filePath)) {
        return;
    }

    bool mirrorDelete;
    {
        std::lock_guard<std::mutex> lock(configMutex_);
        mirrorDelete = config_.mirrorDelete;
    }

    fs::path pathW = pathFromUtf8(filePath);
    switch (action) {
        case FILE_ACTION_ADDED:
        case FILE_ACTION_MODIFIED:
            if (fs::exists(pathW) && !fs::is_directory(pathW)) {
                backupFile(filePath);
            }
            break;

        case FILE_ACTION_REMOVED:
            if (mirrorDelete) {
                deleteMirrorFile(filePath);
            }
            break;

        case FILE_ACTION_RENAMED_OLD_NAME:
            pendingRenameOldPath_ = filePath;
            break;

        case FILE_ACTION_RENAMED_NEW_NAME: {
            // 先删除旧目标（保持镜像一致），再备份新文件
            if (mirrorDelete && !pendingRenameOldPath_.empty()) {
                deleteMirrorFile(pendingRenameOldPath_);
            }
            pendingRenameOldPath_.clear();
            if (fs::exists(pathW) && !fs::is_directory(pathW)
                && shouldBackupFile(filePath)) {
                backupFile(filePath);
            }
            break;
        }

        default: break;
    }
}

std::string RealtimeBackupMonitor::computeTargetPath(const std::string& sourceFilePath) const {
    std::string sourceDir, targetDir;
    {
        std::lock_guard<std::mutex> lock(configMutex_);
        sourceDir = config_.sourcePath;
        targetDir = config_.targetPath;
    }

    fs::path srcFile = pathFromUtf8(sourceFilePath);
    fs::path srcDir  = pathFromUtf8(sourceDir);
    fs::path tgtDir  = pathFromUtf8(targetDir);

    std::error_code ec;
    fs::path absSource = fs::weakly_canonical(srcFile, ec);
    if (ec) absSource = fs::absolute(srcFile);
    fs::path absSourceDir = fs::weakly_canonical(srcDir, ec);
    if (ec) absSourceDir = fs::absolute(srcDir);

    fs::path rel = fs::relative(absSource, absSourceDir, ec);
    if (ec || rel.empty() || rel.native().rfind(L"..", 0) == 0) {
        // 备用：用字符串前缀剥离
        std::wstring s = srcFile.wstring();
        std::wstring base = srcDir.wstring();
        if (!base.empty() && (base.back() == L'\\' || base.back() == L'/')) {
            base.pop_back();
        }
        if (s.rfind(base, 0) == 0) {
            std::wstring tail = s.substr(base.size());
            while (!tail.empty() && (tail.front() == L'\\' || tail.front() == L'/')) {
                tail.erase(0, 1);
            }
            return pathToUtf8(tgtDir / fs::path(tail));
        }
        return pathToUtf8(tgtDir / srcFile.filename());
    }
    return pathToUtf8(tgtDir / rel);
}

bool RealtimeBackupMonitor::isInsideTarget(const std::string& absSource) const {
    std::string targetDir;
    {
        std::lock_guard<std::mutex> lock(configMutex_);
        targetDir = config_.targetPath;
    }
    if (targetDir.empty()) return false;

    std::error_code ec;
    fs::path a = fs::weakly_canonical(pathFromUtf8(absSource), ec);
    if (ec) a = fs::absolute(pathFromUtf8(absSource));
    fs::path t = fs::weakly_canonical(pathFromUtf8(targetDir), ec);
    if (ec) t = fs::absolute(pathFromUtf8(targetDir));

    // 判断 a 是否在 t 之内（含 a == t）
    auto ai = a.begin(), ti = t.begin();
    for (; ti != t.end(); ++ti, ++ai) {
        if (ai == a.end()) return false;
        if (*ai != *ti) return false;
    }
    return true;
}

bool RealtimeBackupMonitor::backupFile(const std::string& filePath) {
    try {
        fs::path srcPathW = pathFromUtf8(filePath);
        if (!fs::exists(srcPathW) || fs::is_directory(srcPathW)) {
            return false;
        }

        // 排除目标目录内的文件（防止自反循环）
        if (isInsideTarget(filePath)) {
            return false;
        }

        std::string targetPath = computeTargetPath(filePath);
        fs::path tgtPathW = pathFromUtf8(targetPath);

        // 创建目标目录
        {
            fs::path parent = tgtPathW.parent_path();
            if (!parent.empty()) {
                std::error_code ec;
                fs::create_directories(parent, ec);
                if (ec) {
                    std::cerr << "[RealtimeBackupMonitor] Failed to create dir: " << ec.message() << std::endl;
                    std::lock_guard<std::mutex> lock(statsMutex_);
                    stats_.backupFailures++;
                    return false;
                }
            }
        }

        // 读取源文件 (用宽字符路径,支持中文路径)
        std::ifstream inFile(srcPathW, std::ios::binary);
        if (!inFile) {
            std::cerr << "[RealtimeBackupMonitor] Failed to open source: " << filePath << std::endl;
            std::lock_guard<std::mutex> lock(statsMutex_);
            stats_.backupFailures++;
            return false;
        }

        std::vector<uint8_t> data((std::istreambuf_iterator<char>(inFile)),
                                  std::istreambuf_iterator<char>());
        inFile.close();

        // 加密（如果启用）
        std::vector<uint8_t> outputData;
        bool encryptionEnabled;
        std::string password;
        {
            std::lock_guard<std::mutex> lock(configMutex_);
            encryptionEnabled = config_.enableEncryption;
            password = config_.encryptionPassword;
        }

        if (encryptionEnabled && encryptor_) {
            if (!encryptor_->encrypt(data, outputData, password)) {
                std::cerr << "[RealtimeBackupMonitor] Encryption failed: " << filePath << std::endl;
                std::lock_guard<std::mutex> lock(statsMutex_);
                stats_.backupFailures++;
                return false;
            }
        } else {
            outputData = std::move(data);
        }

        // 写入目标文件（同名覆盖）
        // 先清除可能存在的只读位
#ifdef _WIN32
        if (fs::exists(tgtPathW)) {
            DWORD attrs = GetFileAttributesW(tgtPathW.wstring().c_str());
            if (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_READONLY)) {
                SetFileAttributesW(tgtPathW.wstring().c_str(), attrs & ~FILE_ATTRIBUTE_READONLY);
            }
        }
#endif
        std::ofstream outFile(tgtPathW, std::ios::binary | std::ios::trunc);
        if (!outFile) {
            std::cerr << "[RealtimeBackupMonitor] Failed to create target: " << targetPath << std::endl;
            std::lock_guard<std::mutex> lock(statsMutex_);
            stats_.backupFailures++;
            return false;
        }
        outFile.write(reinterpret_cast<const char*>(outputData.data()),
                      static_cast<std::streamsize>(outputData.size()));
        outFile.close();

        // 保存元数据(可选)
        try {
            FileInfo info = metadataManager_->extractMetadata(srcPathW);
            fs::path metaPath = tgtPathW;
            metaPath += L".meta";
            metadataManager_->saveMetadataToFile(info, metaPath);
        } catch (...) {
            // 元数据失败不视为整体失败
        }

        {
            std::lock_guard<std::mutex> lock(statsMutex_);
            stats_.filesBackedUp++;
            stats_.totalBytesBackedUp += outputData.size();
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << "[RealtimeBackupMonitor] Backup failed: " << e.what() << std::endl;
        std::lock_guard<std::mutex> lock(statsMutex_);
        stats_.backupFailures++;
        return false;
    }
}

bool RealtimeBackupMonitor::deleteMirrorFile(const std::string& sourceFilePath) {
    try {
        std::string targetPath = computeTargetPath(sourceFilePath);
        fs::path tgtW = pathFromUtf8(targetPath);
        std::error_code ec;

        bool removedAny = false;
        if (fs::exists(tgtW, ec)) {
#ifdef _WIN32
            DWORD attrs = GetFileAttributesW(tgtW.wstring().c_str());
            if (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_READONLY)) {
                SetFileAttributesW(tgtW.wstring().c_str(), attrs & ~FILE_ATTRIBUTE_READONLY);
            }
#endif
            if (fs::is_directory(tgtW, ec)) {
                fs::remove_all(tgtW, ec);
            } else {
                fs::remove(tgtW, ec);
            }
            removedAny = !ec;
        }

        // 同时清理 .meta
        fs::path metaW = pathFromUtf8(targetPath + ".meta");
        if (fs::exists(metaW, ec)) {
            fs::remove(metaW, ec);
        }

        if (removedAny) {
            std::lock_guard<std::mutex> lock(statsMutex_);
            stats_.filesDeleted++;
        }
        return removedAny;
    } catch (const std::exception& e) {
        std::cerr << "[RealtimeBackupMonitor] Delete mirror failed: " << e.what() << std::endl;
        return false;
    }
}

bool RealtimeBackupMonitor::shouldBackupFile(const std::string& filePath) {
    std::vector<std::string> filters;
    {
        std::lock_guard<std::mutex> lock(configMutex_);
        filters = config_.fileFilters;
    }
    if (filters.empty()) return true;

    // 获取扩展名（无点），转小写
    std::string ext = pathToUtf8(pathFromUtf8(filePath).extension());
    if (!ext.empty() && ext.front() == '.') ext.erase(0, 1);
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    for (const auto& filter : filters) {
        std::string f = filter;
        // 兼容 "*.txt"、".txt"、"txt"
        if (!f.empty() && f.front() == '*') f.erase(0, 1);
        if (!f.empty() && f.front() == '.') f.erase(0, 1);
        std::transform(f.begin(), f.end(), f.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (!f.empty() && ext == f) return true;
    }
    return false;
}

uint64_t RealtimeBackupMonitor::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

} // namespace DataBackup
