# 数据备份软件 - 接口文档

本文档记录所有功能模块的接口定义，用于 GUI 开发快速对接。

---

## 快速开始

### 头文件包含

```cpp
#include "core/FileInfo.h"
#include "core/FileScanner.h"
#include "core/MetadataManager.h"
#include "pack/Packager.h"
#include "filter/FilterChain.h"
#include "filter/PathFilter.h"
#include "filter/TypeFilter.h"
#include "filter/NameFilter.h"
#include "filter/TimeFilter.h"
#include "filter/SizeFilter.h"
#include "compression/ICompressionStrategy.h"
#include "compression/HuffmanCompression.h"
#include "compression/ZlibCompression.h"  // 可选，需要安装zlib库
#include "encryption/IEncryptionStrategy.h"
#include "encryption/XOREncryption.h"
#include "engine/BackupEngine.h"
#include "engine/RestoreEngine.h"
#include "engine/BackupConfig.h"
#include "scheduler/BackupScheduler.h"
#include "scheduler/ScheduleTypes.h"
#include "realtime/RealtimeBackupMonitor.h"
#include "realtime/RealtimeBackupTypes.h"
```

### 命名空间

所有类位于 `DataBackup` 命名空间下：

```cpp
using namespace DataBackup;

// 或者显式使用
DataBackup::FileScanner scanner;
DataBackup::Packager packager;
```

### 错误处理

所有模块采用以下错误处理策略：
- **返回值**：`bool` 类型方法返回 `true` 表示成功，`false` 表示失败
- **异常**：文件不存在、权限不足等严重错误抛出 `std::runtime_error`
- **日志**：错误信息通过 `std::cerr` 输出（后续可集成日志系统）

## 目录

1. [FileInfo - 文件信息实体](#fileinfo---文件信息实体)
2. [FileScanner - 文件扫描器](#filescanner---文件扫描器)
3. [MetadataManager - 元数据管理器](#metadatamanager---元数据管理器)
4. [Packager - 文件打包器](#packager---文件打包器)
5. [Filter - 文件过滤器链](#文件过滤器链)
6. [Compression - 压缩模块](#压缩模块)
7. [Encryption - 加密模块](#加密模块)
8. [Backup Engine - 备份引擎](#备份引擎)
9. [Scheduler - 定时调度器](#定时调度器)
10. [Realtime Backup Monitor - 实时备份监控](#实时备份监控)

---

## FileInfo - 文件信息实体

**头文件**：`src/core/FileInfo.h`

### 类定义

```cpp
class FileInfo {
public:
    fs::path path;                    // 文件路径
    uint64_t size = 0;                // 文件大小（字节）
    bool isDirectory = false;         // 是否为目录

    bool isHardlink = false;          // 是否为硬链接
    bool isSymlink = false;           // 是否为符号链接
    fs::path hardlinkTarget;          // 硬链接目标
    uint32_t hardlinkCount = 1;       // 硬链接引用计数
    fs::path symlinkTarget;           // 符号链接目标

    uint64_t creationTime = 0;        // 创建时间（FILETIME）
    uint64_t lastWriteTime = 0;       // 修改时间
    uint64_t lastAccessTime = 0;      // 访问时间
    uint32_t attributes = 0;          // Windows 文件属性
    std::string permissions;          // 权限字符串

    FileInfo() = default;
    explicit FileInfo(const fs::path& filePath);
};
```

### 使用示例

```cpp
FileInfo info1;
FileInfo info2("C:\\Users\\test\\file.txt");

info2.size = 1024;
info2.creationTime = 132456789012345678;
```

---

## FileScanner - 文件扫描器

**头文件**：`src/core/FileScanner.h`

### 类定义

```cpp
class FileScanner {
public:
    struct ScanConfig {
        bool followSymlinks = false;
        bool skipHardlinks = true;
        size_t maxDepth = 100;
    };

    struct ScanStatistics {
        uint32_t totalFiles = 0;
        uint32_t totalDirectories = 0;
        uint32_t hardlinksFound = 0;
        uint32_t symlinksFound = 0;
        uint64_t totalSize = 0;
    };

    std::vector<FileInfo> scan(const fs::path& rootPath,
                              const ScanConfig& config = ScanConfig{});
    ScanStatistics getStatistics() const;
};
```

### 使用示例

```cpp
FileScanner scanner;
FileScanner::ScanConfig config;
config.followSymlinks = false;
config.skipHardlinks = true;

std::vector<FileInfo> files = scanner.scan("C:\\Users\\test\\Documents", config);
auto stats = scanner.getStatistics();
```

---

## MetadataManager - 元数据管理器

**头文件**：`src/core/MetadataManager.h`

### 类定义

```cpp
class MetadataManager {
public:
    bool extractMetadata(const fs::path& filePath, FileInfo& fileInfo);
    bool setMetadata(const fs::path& filePath, const FileInfo& fileInfo);
    std::vector<FileInfo> extractBatch(const std::vector<fs::path>& filePaths);

    std::string serialize(const FileInfo& fileInfo);
    bool deserialize(const std::string& json, FileInfo& fileInfo);

    bool saveToFile(const fs::path& filePath, const FileInfo& fileInfo);
    bool loadFromFile(const fs::path& filePath, FileInfo& fileInfo);

    std::string getPermissions(const fs::path& filePath);
};
```

### 使用示例

```cpp
MetadataManager manager;

// 提取元数据
FileInfo fileInfo;
manager.extractMetadata("C:\\test\\file.txt", fileInfo);

// 设置元数据
manager.setMetadata("C:\\restore\\file.txt", fileInfo);

// 序列化
std::string json = manager.serialize(fileInfo);
```

---

## Packager - 文件打包器

**头文件**：`src/pack/Packager.h`

### 类定义

```cpp
class IPackager {
public:
    virtual ~IPackager() = default;

    virtual bool pack(const std::vector<FileInfo>& files,
                     const fs::path& packagePath,
                     std::function<void(int, int)> progressCallback = nullptr,
                     const fs::path& basePath = "") = 0;

    virtual bool unpack(const fs::path& packagePath,
                       const fs::path& outputDir,
                       std::function<void(int, int)> progressCallback = nullptr) = 0;

    virtual PackageInfo getPackageInfo(const fs::path& packagePath) = 0;
    virtual bool verifyPackage(const fs::path& packagePath) = 0;
};

struct PackageInfo {
    uint32_t version;
    uint32_t fileCount;
    uint64_t totalSize;
    uint64_t checksum;
    std::vector<std::string> filePaths;
};

class Packager : public IPackager { /* 实现 */ };
```

### 使用示例

```cpp
Packager packager;

// 打包
auto progressCallback = [](int current, int total) {
    std::cout << "进度: " << current << "/" << total << std::endl;
};

packager.pack(files, "backup.pkg", progressCallback, "C:\\Users\\test");

// 解包
packager.unpack("backup.pkg", "C:\\Restore", progressCallback);

// 获取包信息
PackageInfo info = packager.getPackageInfo("backup.pkg");
```

---

## 文件过滤器链

### IFileFilter - 过滤器接口

**头文件**：`src/filter/IFileFilter.h`

```cpp
class IFileFilter {
public:
    virtual ~IFileFilter() = default;
    virtual bool accept(const FileInfo& fileInfo) const = 0;
    virtual std::string getName() const = 0;
    virtual std::string getDescription() const = 0;
};
```

### FilterChain - 过滤器链

**头文件**：`src/filter/FilterChain.h`

```cpp
class FilterChain : public IFileFilter {
public:
    enum class Mode {
        AND,  // 所有过滤器都必须接受
        OR    // 任意一个过滤器接受即可
    };

    explicit FilterChain(Mode mode = Mode::AND);
    void addFilter(std::shared_ptr<IFileFilter> filter);
    void clearFilters();
    size_t getFilterCount() const;

    bool accept(const FileInfo& fileInfo) const override;
};
```

**使用示例**：
```cpp
FilterChain chain(FilterChain::Mode::AND);
chain.addFilter(std::make_shared<PathFilter>(PathFilter({"C:\\Users"}, {})));
chain.addFilter(std::make_shared<TypeFilter>(TypeFilter({"txt", "pdf"}, {})));
chain.addFilter(std::make_shared<SizeFilter>(SizeFilter(0, 10 * 1024 * 1024)));

for (const auto& file : files) {
    if (chain.accept(file)) {
        // 文件通过所有过滤器
    }
}
```

### PathFilter - 路径过滤

**头文件**：`src/filter/PathFilter.h`

```cpp
class PathFilter : public IFileFilter {
public:
    PathFilter(const std::vector<std::string>& includePaths = {},
               const std::vector<std::string>& excludePaths = {});

    void addIncludePath(const std::string& path);
    void addExcludePath(const std::string& path);

    bool accept(const FileInfo& fileInfo) const override;
};
```

**使用示例**：
```cpp
PathFilter filter(
    {"C:\\Users\\test\\Documents"},  // 包含路径
    {"C:\\Users\\test\\Documents\\Temp"}  // 排除路径
);
```

### TypeFilter - 类型过滤

**头文件**：`src/filter/TypeFilter.h`

```cpp
class TypeFilter : public IFileFilter {
public:
    TypeFilter(const std::vector<std::string>& includeTypes = {},
               const std::vector<std::string>& excludeTypes = {});

    void addIncludeType(const std::string& type);
    void addExcludeType(const std::string& type);

    bool accept(const FileInfo& fileInfo) const override;
};
```

**使用示例**：
```cpp
TypeFilter filter(
    {"txt", "pdf", "doc"},  // 包含类型
    {"tmp", "bak"}  // 排除类型
);
```

### NameFilter - 文件名过滤

**头文件**：`src/filter/NameFilter.h`

```cpp
class NameFilter : public IFileFilter {
public:
    explicit NameFilter(const std::string& pattern, bool useRegex = false);
    void setPattern(const std::string& pattern);

    bool accept(const FileInfo& fileInfo) const override;
};
```

**使用示例**：
```cpp
NameFilter filter1("*.txt");  // 通配符匹配
NameFilter filter2("doc[0-9]+\\.txt", true);  // 正则表达式
```

**通配符说明**：
- `*` 匹配0个或多个字符
- `?` 匹配恰好1个字符

### TimeFilter - 时间过滤

**头文件**：`src/filter/TimeFilter.h`

```cpp
class TimeFilter : public IFileFilter {
public:
    enum class TimeType {
        CREATION,     // 创建时间
        LAST_WRITE,   // 修改时间
        LAST_ACCESS   // 访问时间
    };

    TimeFilter(TimeType timeType, uint64_t startTime = 0, uint64_t endTime = 0);
    void setTimeRange(uint64_t startTime, uint64_t endTime);

    bool accept(const FileInfo& fileInfo) const override;
};
```

**使用示例**：
```cpp
TimeFilter filter(
    TimeFilter::TimeType::LAST_WRITE,
    startTime,  // FILETIME格式
    endTime
);
```

### SizeFilter - 大小过滤

**头文件**：`src/filter/SizeFilter.h`

```cpp
class SizeFilter : public IFileFilter {
public:
    SizeFilter(uint64_t minSize = 0, uint64_t maxSize = 0);
    void setSizeRange(uint64_t minSize, uint64_t maxSize);

    static SizeFilter fromReadable(const std::string& minSize = "0B",
                                    const std::string& maxSize = "0B");

    bool accept(const FileInfo& fileInfo) const override;
};
```

**使用示例**：
```cpp
SizeFilter filter1(0, 100 * 1024 * 1024);  // 0-100MB
SizeFilter filter2 = SizeFilter::fromReadable("1KB", "10MB");
```

---

## Compression - 压缩模块

### ICompressionStrategy - 压缩策略接口

**头文件**：`src/compression/ICompressionStrategy.h`

```cpp
class ICompressionStrategy {
public:
    virtual ~ICompressionStrategy() = default;

    virtual bool compress(const std::vector<uint8_t>& input,
                          std::vector<uint8_t>& output) = 0;

    virtual bool decompress(const std::vector<uint8_t>& input,
                             std::vector<uint8_t>& output) = 0;

    virtual std::string getName() const = 0;
};
```

### HuffmanCompression - Huffman压缩实现

**头文件**：`src/compression/HuffmanCompression.h`

```cpp
class HuffmanCompression : public ICompressionStrategy {
public:
    HuffmanCompression();
    ~HuffmanCompression();

    bool compress(const std::vector<uint8_t>& input,
                  std::vector<uint8_t>& output) override;

    bool decompress(const std::vector<uint8_t>& input,
                    std::vector<uint8_t>& output) override;

    std::string getName() const override;
};
```

**使用示例**：
```cpp
HuffmanCompression compressor;

// 压缩数据
std::vector<uint8_t> originalData = readFile("file.txt");
std::vector<uint8_t> compressedData;
compressor.compress(originalData, compressedData);

// 解压数据
std::vector<uint8_t> decompressedData;
compressor.decompress(compressedData, decompressedData);
```

**算法特点**：
- **无损压缩**：完美还原原始数据
- **自适应编码**：基于字节频率构建最优编码
- **适合文本数据**：对字符频率分布不均匀的文本压缩效果好（30-50%压缩率）
- **单字节支持**：正确处理单字节数据的边界情况
- **无外部依赖**：完全自实现，无需第三方库

**压缩数据格式**：
```
[原始数据大小（8字节）]
+ [Huffman树序列化]
+ [编码比特数量（8字节）]
+ [编码后的比特流]
```

### ZlibCompression - Zlib压缩实现（可选）

**头文件**：`src/compression/ZlibCompression.h`

**依赖**：需要安装zlib库（详见构建说明）

```cpp
class ZlibCompression : public ICompressionStrategy {
public:
    explicit ZlibCompression(int compressionLevel = -1);

    bool compress(const std::vector<uint8_t>& input,
                  std::vector<uint8_t>& output) override;

    bool decompress(const std::vector<uint8_t>& input,
                    std::vector<uint8_t>& output) override;

    std::string getName() const override { return "Zlib"; }

    void setCompressionLevel(int level);
    int getCompressionLevel() const;
};
```

**使用示例**：
```cpp
ZlibCompression compressor(6);  // 压缩级别6（默认）

std::vector<uint8_t> originalData = readFile("file.txt");
std::vector<uint8_t> compressedData;
compressor.compress(originalData, compressedData);

std::vector<uint8_t> decompressedData;
compressor.decompress(compressedData, decompressedData);
```

**压缩级别**：
- `-1`：默认压缩（Z_DEFAULT_COMPRESSION）
- `0`：无压缩（仅存储）
- `1-9`：压缩级别，1最快，9最高压缩率

### 算法选择建议

| 算法 | 适用场景 | 优点 | 缺点 |
|------|---------|------|------|
| **Huffman** | 文本数据、无需外部依赖 | 自实现、跨平台 | 压缩率较低 |
| **Zlib** | 通用数据、已安装zlib | 高压缩率、速度快 | 需要外部库 |

**示例：动态选择算法**
```cpp
std::unique_ptr<ICompressionStrategy> createCompressor(bool useZlib) {
    if (useZlib) {
        return std::make_unique<ZlibCompression>(6);
    } else {
        return std::make_unique<HuffmanCompression>();
    }
}
```

---

## 加密模块

### IEncryptionStrategy - 加密策略接口

**头文件**：`src/encryption/IEncryptionStrategy.h`

```cpp
class IEncryptionStrategy {
public:
    virtual ~IEncryptionStrategy() = default;

    virtual bool encrypt(const std::vector<uint8_t>& plaintext,
                         std::vector<uint8_t>& ciphertext,
                         const std::string& password) = 0;

    virtual bool decrypt(const std::vector<uint8_t>& ciphertext,
                         std::vector<uint8_t>& plaintext,
                         const std::string& password) = 0;

    virtual std::string getName() const = 0;
};
```

### XOREncryption - XOR加密实现

**头文件**：`src/encryption/XOREncryption.h`

```cpp
class XOREncryption : public IEncryptionStrategy {
public:
    XOREncryption();

    bool encrypt(const std::vector<uint8_t>& plaintext,
                 std::vector<uint8_t>& ciphertext,
                 const std::string& password) override;

    bool decrypt(const std::vector<uint8_t>& ciphertext,
                 std::vector<uint8_t>& plaintext,
                 const std::string& password) override;

    std::string getName() const override { return "XOR"; }
};
```

**使用示例**：
```cpp
XOREncryption encryption;

// 加密数据
std::vector<uint8_t> plaintext = readFile("file.txt");
std::vector<uint8_t> ciphertext;
encryption.encrypt(plaintext, ciphertext, "my_password");

// 解密数据
std::vector<uint8_t> decrypted;
encryption.decrypt(ciphertext, decrypted, "my_password");
```

**算法特点**：
- **对称加密**：加密和解密使用相同的密码
- **速度快**：O(n)复杂度，适合大文件
- **无外部依赖**：完全自实现
- **适合学习**：理解加密基本原理
- **安全性较低**：不适合高安全场景

---

## 定时调度器

### BackupScheduler - 备份调度器

**头文件**：`src/scheduler/BackupScheduler.h`

```cpp
class BackupScheduler : public IScheduler {
public:
    // 任务管理
    bool addTask(const ScheduleConfig& config);
    bool removeTask(const std::string& taskId);
    bool pauseTask(const std::string& taskId);
    bool resumeTask(const std::string& taskId);

    // 调度器控制
    void start();
    void stop();
    bool isRunning() const;

    // 任务信息
    std::vector<TaskInfo> getTaskList() const;
    TaskInfo getTaskInfo(const std::string& taskId) const;

    // 回调设置
    void setTaskCallback(const std::string& taskId, TaskCallback callback);

    // 配置管理
    bool saveTasks(const std::string& filePath);
    bool loadTasks(const std::string& filePath);

    // 手动执行
    TaskExecutionResult executeTask(const std::string& taskId);
};
```

**使用示例**：
```cpp
BackupScheduler scheduler;

// 添加每天23:59执行的备份任务
ScheduleConfig config;
config.taskId = "BACKUP_001";
config.taskName = "Daily Backup";
config.type = ScheduleType::DAILY;
config.hour = 23;
config.minute = 59;
config.backupConfig.sourcePath = "C:\\Users\\Documents";
config.backupConfig.targetPath = "D:\\Backups\\daily_backup.pkg";

scheduler.addTask(config);

// 启动调度器
scheduler.start();

// 停止调度器
scheduler.stop();
```

---

## 实时备份监控

### RealtimeBackupConfig - 实时备份配置

**头文件**：`src/realtime/RealtimeBackupTypes.h`

```cpp
struct RealtimeBackupConfig {
    std::string sourcePath;           // 监控的源目录
    std::string targetPath;           // 备份目标目录
    bool enableEncryption = false;    // 是否启用加密
    std::string encryptionPassword;   // 加密密码
    bool includeSubdirectories = true; // 是否包含子目录
    std::vector<std::string> fileFilters; // 文件过滤器（扩展名）
};
```

### FileChangeType - 文件变化类型

```cpp
enum class FileChangeType {
    CREATED,    // 文件创建
    MODIFIED,   // 文件修改
    DELETED,    // 文件删除
    RENAMED     // 文件重命名
};
```

### RealtimeBackupStatus - 实时备份状态

```cpp
enum class RealtimeBackupStatus {
    IDLE,       // 空闲
    RUNNING,    // 运行中
    PAUSED,     // 已暂停
    ERROR_STATE // 错误状态
};
```

### FileChangeEvent - 文件变化事件

```cpp
struct FileChangeEvent {
    FileChangeType type;      // 变化类型
    std::string filePath;     // 文件路径
    std::string oldFilePath;  // 旧文件路径（重命名时有效）
    uint64_t timestamp;       // 时间戳
};
```

### RealtimeBackupStats - 实时备份统计

```cpp
struct RealtimeBackupStats {
    uint32_t filesMonitored = 0;      // 监控的文件数
    uint32_t filesBackedUp = 0;       // 已备份文件数
    uint32_t backupFailures = 0;      // 备份失败次数
    uint64_t totalBytesBackedUp = 0;  // 总备份字节数
    uint64_t startTime = 0;           // 启动时间
};
```

### IRealtimeBackupMonitor - 实时备份监控器接口

**头文件**：`src/realtime/IRealtimeBackupMonitor.h`

```cpp
using FileChangeCallback = std::function<void(const FileChangeEvent&)>;

class IRealtimeBackupMonitor {
public:
    virtual ~IRealtimeBackupMonitor() = default;

    virtual bool startMonitor(const RealtimeBackupConfig& config,
                              FileChangeCallback callback = nullptr) = 0;
    virtual void stopMonitor() = 0;
    virtual void pauseMonitor() = 0;
    virtual void resumeMonitor() = 0;

    virtual RealtimeBackupStatus getStatus() const = 0;
    virtual RealtimeBackupStats getStats() const = 0;
    virtual std::string getMonitoredPath() const = 0;

    virtual bool backupFileNow(const std::string& filePath) = 0;
};
```

### RealtimeBackupMonitor - 实时备份监控器实现

**头文件**：`src/realtime/RealtimeBackupMonitor.h`

```cpp
class RealtimeBackupMonitor : public IRealtimeBackupMonitor {
public:
    RealtimeBackupMonitor();
    ~RealtimeBackupMonitor() override;

    // IRealtimeBackupMonitor接口实现
    bool startMonitor(const RealtimeBackupConfig& config,
                      FileChangeCallback callback = nullptr) override;

    void stopMonitor() override;
    void pauseMonitor() override;
    void resumeMonitor() override;

    RealtimeBackupStatus getStatus() const override;
    RealtimeBackupStats getStats() const override;
    std::string getMonitoredPath() const override;
    bool isRunning() const;

    bool backupFileNow(const std::string& filePath) override;
};
```

**使用示例**：

```cpp
#include "realtime/RealtimeBackupMonitor.h"
using namespace DataBackup;

// 创建实时备份监控器
RealtimeBackupMonitor monitor;

// 配置实时备份
RealtimeBackupConfig config;
config.sourcePath = "C:\\Users\\Documents";
config.targetPath = "D:\\RealtimeBackup";
config.enableEncryption = true;
config.encryptionPassword = "my_password";

// 设置文件变化回调（可选）
FileChangeCallback callback = [](const FileChangeEvent& event) {
    std::cout << "File changed: " << event.filePath << std::endl;
};

// 启动监控
if (monitor.startMonitor(config, callback)) {
    std::cout << "Monitoring started" << std::endl;
}

// 获取统计信息
RealtimeBackupStats stats = monitor.getStats();
std::cout << "Files backed up: " << stats.filesBackedUp << std::endl;

// 手动备份单个文件
monitor.backupFileNow("C:\\Users\\Documents\\file.txt");

// 暂停监控
monitor.pauseMonitor();

// 恢复监控
monitor.resumeMonitor();

// 停止监控
monitor.stopMonitor();
```

**功能特点**：

- **Windows API监控**：使用 `ReadDirectoryChangesW` 监控文件变化
- **实时备份**：文件创建或修改时自动备份
- **保留目录结构**：备份时保留源目录结构
- **可选加密**：支持XOR加密备份
- **文件过滤**：支持按扩展名过滤
- **线程安全**：多线程安全设计
- **暂停/恢复**：支持暂停和恢复监控

**注意事项**：

- 实时备份不打包文件，直接复制到目标目录
- 支持中文路径（UTF-8编码）
- 元数据保存在 `.meta` 文件中
- 定时备份可以打包（与实时备份不同）

---

## 备份引擎

### BackupConfig - 备份配置

**头文件**：`src/engine/BackupConfig.h`

```cpp
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

    // 其他配置
    bool preserveDirectoryStructure = true;  // 保留目录结构
    bool verifyAfterBackup = true;           // 备份后验证
    bool skipHardlinks = true;               // 跳过硬链接
    bool followSymlinks = false;             // 跟随符号链接
};
```

### BackupEngine - 备份引擎

**头文件**：`src/engine/BackupEngine.h`

```cpp
class BackupEngine {
public:
    using ProgressCallback = std::function<void(const BackupProgress&)>;

    BackupEngine();
    ~BackupEngine();

    BackupResult createBackup(const BackupConfig& config,
                              ProgressCallback progressCallback = nullptr);

    bool cancelBackup();
    BackupProgress getProgress() const;
    void setProgressCallback(ProgressCallback callback);
    bool isRunning() const;
};
```

**使用示例**：
```cpp
BackupEngine engine;
BackupConfig config("C:\\Users\\Documents", "D:\\Backups\\backup.pkg");

// 可选：添加过滤器
auto txtFilter = std::make_shared<TypeFilter>(std::vector<std::string>{"txt"}, std::vector<std::string>{});
config.filters.push_back(txtFilter);

// 可选：设置压缩和加密
config.compressionAlgorithm = "huffman";
config.encryptionAlgorithm = "xor";
config.password = "my_password";

// 创建备份
BackupResult result = engine.createBackup(config);
if (result.success) {
    std::cout << "Backup completed: " << result.filesBackedup << " files" << std::endl;
    std::cout << "Duration: " << result.duration << " ms" << std::endl;
}
```

### RestoreEngine - 还原引擎

**头文件**：`src/engine/RestoreEngine.h`

```cpp
class RestoreEngine {
public:
    using ProgressCallback = std::function<void(const BackupProgress&)>;

    RestoreEngine();
    ~RestoreEngine();

    RestoreResult restore(const std::string& backupFilePath,
                          const std::string& targetDirectory,
                          const std::string& password = "",
                          ProgressCallback progressCallback = nullptr);

    bool verifyBackup(const std::string& backupFilePath);
    std::string getBackupInfo(const std::string& backupFilePath);
    bool cancelRestore();
    BackupProgress getProgress() const;
};
```

**使用示例**：
```cpp
RestoreEngine engine;

// 还原备份
RestoreResult result = engine.restore("D:\\Backups\\backup.pkg", "C:\\Restored", "my_password");
if (result.success) {
    std::cout << "Restore completed: " << result.filesRestored << " files" << std::endl;
}

// 验证备份文件
bool isValid = engine.verifyBackup("D:\\Backups\\backup.pkg");

// 获取备份信息
std::string info = engine.getBackupInfo("D:\\Backups\\backup.pkg");
```

### BackupResult - 备份结果

```cpp
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

    double getCompressionRatio() const;  // 获取压缩率
    double getSpaceSaving() const;       // 获取空间节省率
};
```

### BackupProgress - 备份进度

```cpp
enum class BackupPhase {
    IDLE,               // 空闲状态
    SCANNING,           // 扫描文件
    FILTERING,          // 过滤文件
    COMPRESSING,        // 压缩数据
    ENCRYPTING,         // 加密数据
    PACKING,            // 打包文件
    VERIFYING,          // 验证备份
    COMPLETED,          // 完成
    FAILED,             // 失败
    CANCELLED           // 已取消
};

struct BackupProgress {
    BackupPhase phase = BackupPhase::IDLE;
    std::string currentFile;
    uint32_t filesProcessed = 0;
    uint32_t totalFiles = 0;
    uint64_t bytesProcessed = 0;
    uint64_t totalBytes = 0;
    uint8_t percentage = 0;

    std::string getPhaseName() const;
};
```

---

## 完整备份还原流程示例

### 备份流程

```cpp
// 1. 扫描源目录
FileScanner scanner;
std::vector<FileInfo> files = scanner.scan(sourceDir);

// 2. 应用过滤器（可选）
FilterChain chain(FilterChain::Mode::AND);
chain.addFilter(std::make_shared<TypeFilter>(TypeFilter({"txt", "pdf"}, {})));

std::vector<FileInfo> filteredFiles;
for (const auto& file : files) {
    if (chain.accept(file)) {
        filteredFiles.push_back(file);
    }
}

// 3. 提取元数据
MetadataManager metaManager;
std::vector<FileInfo> filesWithMetadata = metaManager.extractBatch(filteredFiles);

// 4. 打包文件
Packager packager;
packager.pack(filesWithMetadata, packagePath, progressCallback, sourceDir);
```

### 还原流程

```cpp
// 1. 解包文件
Packager packager;
packager.unpack(packagePath, restoreDir, progressCallback);

// 2. 恢复元数据
MetadataManager metaManager;
std::vector<fs::path> restoredFiles = listRestoredFiles(restoreDir);
for (const auto& file : restoredFiles) {
    FileInfo metadata = loadMetadataFromBackup(file);
    metaManager.setMetadata(file, metadata);
}
```

---

## 模块依赖关系

```
┌─────────────────┐
│    FileInfo     │  (基础实体类)
└────────┬────────┘
         │
    ┌────┴────┬──────────┬──────────┐
    │         │          │          │
┌───▼───┐ ┌───▼───┐  ┌───▼───┐  ┌───▼───┐
│File   │ │Meta   │  │Packager│ │Filter │
│Scanner│ │Manager│  │       │  │Chain  │
└───────┘ └───────┘  └───────┘  └───────┘
```

**依赖说明**：
- **FileInfo**：基础实体类，无依赖
- **FileScanner**：依赖 FileInfo
- **MetadataManager**：依赖 FileInfo
- **Packager**：依赖 FileInfo
- **FilterChain**：依赖 FileInfo 和 IFileFilter 接口

---

## 线程安全性

### 线程安全
- **FileInfo**：线程安全（只读访问）
- **FileScanner::getStatistics()**：线程安全（只读）
- **FilterChain**：线程安全（配置完成后只读访问）

### 非线程安全
- **FileScanner::scan()**：非线程安全，不应并发调用
- **MetadataManager**：非线程安全，实例不应跨线程共享
- **Packager**：非线程安全，实例不应跨线程共享

**建议**：GUI 开发时，每个备份任务使用独立的实例，避免跨线程共享。

---

## 性能注意事项

### FileScanner
- 使用广度优先遍历，内存占用与目录深度无关
- 对于大型目录（>10万文件），建议：
  - 设置合理的 `maxDepth` 限制递归深度
  - 在后台线程执行扫描
  - 使用进度回调通知用户

### Packager
- 打包大文件（>1GB）时内存占用较高
- 建议使用进度回调显示进度条
- 打包前检查磁盘空间是否充足

### FilterChain
- 过滤器链性能与过滤器数量成正比
- 建议将高拒绝率的过滤器放在前面（如 SizeFilter）
- 使用 `Mode::AND` 时，第一个拒绝的过滤器会短路后续检查

---

## 常见问题 (FAQ)

### Q1: 如何处理中文路径？
所有模块使用 `std::filesystem::path`，原生支持 Unicode 路径，无需特殊处理。

### Q2: 如何获取用户友好的文件大小显示？
```cpp
std::string formatSize(uint64_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unitIndex = 0;
    double size = static_cast<double>(bytes);
    while (size >= 1024.0 && unitIndex < 4) {
        size /= 1024.0;
        unitIndex++;
    }
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << size << " " << units[unitIndex];
    return oss.str();
}
```

### Q3: 如何转换 FILETIME 到可读格式？
```cpp
std::string formatFileTime(uint64_t filetime) {
    FILETIME ft;
    ft.dwLowDateTime = filetime & 0xFFFFFFFF;
    ft.dwHighDateTime = filetime >> 32;

    SYSTEMTIME st;
    FileTimeToSystemTime(&ft, &st);

    std::ostringstream oss;
    oss << st.wYear << "-" << std::setfill('0') << std::setw(2) << st.wMonth
        << "-" << std::setw(2) << st.wDay << " "
        << std::setw(2) << st.wHour << ":" << std::setw(2) << st.wMinute
        << ":" << std::setw(2) << st.wSecond;
    return oss.str();
}
```

### Q4: 如何取消正在进行的备份操作？
当前版本不支持取消操作。建议在 GUI 中提供"取消"按钮，下次开发备份引擎时实现中断机制。

### Q5: 硬链接和符号链接如何处理？
- **硬链接**：`FileInfo::isHardlink` 标识，`hardlinkCount` 记录引用次数
- **符号链接**：`FileInfo::isSymlink` 标识，`symlinkTarget` 记录目标路径
- **FileScanner::ScanConfig** 提供配置选项：
  - `skipHardlinks = true`：跳过硬链接（避免重复备份）
  - `followSymlinks = false`：不跟随符号链接

---

## 打包文件格式

Packager 使用自定义格式存储文件，格式说明：

```
┌───────────────────────────────────────┐
│           Package Header (32字节)      │
│  - Magic Number: "DPKG" (4字节)       │
│  - Version: 1 (4字节)                  │
│  - File Count (4字节)                  │
│  - Total Size (8字节)                  │
│  - Index Offset (8字节)                │
│  - Reserved (4字节)                    │
├───────────────────────────────────────┤
│              File Index               │
│  For each file:                       │
│  - Path Length (4字节)                 │
│  - Path String (变长)                  │
│  - File Size (8字节)                   │
│  - Data Offset (8字节)                 │
│  - Metadata JSON Length (4字节)        │
│  - Metadata JSON (变长)                │
├───────────────────────────────────────┤
│              File Data                │
│  For each file:                       │
│  - File Content (变长)                  │
└───────────────────────────────────────┘
```

---

## 版本兼容性

### 当前版本：v1.0

| 模块 | 版本 | 向后兼容 |
|------|------|----------|
| FileInfo | 1.0 | 是 |
| FileScanner | 1.0 | 是 |
| MetadataManager | 1.0 | 是 |
| Packager | 1.0 | 是（包格式版本 1） |
| FilterChain | 1.0 | 是 |

### 后续版本规划
- **v1.1**：压缩模块（Zlib、LZMA）
- **v1.2**：加密模块（AES、XOR）
- **v1.3**：备份引擎集成

---

**文档更新记录**：
- 2026-07-10：创建接口文档
- 2026-07-10：补充文件过滤器链接口
- 2026-07-12：补充快速开始、错误处理、线程安全、性能注意事项、FAQ、打包格式
- 2026-07-13：补充压缩模块、加密模块、备份引擎、定时调度器接口
- 2026-07-13：补充实时备份监控模块接口（RealtimeBackupMonitor）