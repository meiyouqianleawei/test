#ifndef PACKAGER_H
#define PACKAGER_H

#include <string>
#include <vector>
#include <filesystem>
#include <functional>
#include <memory>
#include "../core/FileInfo.h"  // 使用相对路径
#include "../encryption/IEncryptionStrategy.h"

namespace fs = std::filesystem;

/**
 * @brief 包文件信息结构
 */
struct PackageInfo {
    uint32_t fileCount = 0;      ///< 文件数量
    uint64_t totalSize = 0;      ///< 总大小（字节）
    std::string version;          ///< 包格式版本
    uint64_t checksum = 0;        ///< 校验和
};

/**
 * @brief 文件打包器接口
 *
 * 负责将多个文件打包为一个连续文件，提升 I/O 效率。
 * 支持文件元数据保存与还原，支持完整性验证。
 * 
 * 使用场景：
 * - 普通备份和定时备份：使用打包功能
 * - 实时备份（GUI单独按钮）：不使用打包，直接备份文件
 */
class IPackager {
public:
    virtual ~IPackager() = default;

    /**
     * @brief 打包文件
     * @param files 要打包的文件列表
     * @param packagePath 包文件路径
     * @param progressCallback 进度回调函数（可选）
     * @param basePath 基础路径（可选），用于将绝对路径转换为相对路径
     * @return bool 成功返回 true，失败返回 false
     */
    virtual bool pack(const std::vector<FileInfo>& files,
                     const fs::path& packagePath,
                     std::function<void(int, int)> progressCallback = nullptr,
                     const fs::path& basePath = "",
                     IEncryptionStrategy* encryptor = nullptr,
                     const std::string& password = "",
                     class ICompressionStrategy* compressor = nullptr) = 0;

    /**
     * @brief 解包文件
     * @param packagePath 包文件路径
     * @param outputDir 输出目录
     * @param progressCallback 进度回调函数（可选）
     * @param password 解密密码（可选），只有加密包才需要
     * @param outErrorCode 输出错误码（可选）:0=成功;1=密码错误;2=IO/格式错误
     * @return bool 成功返回 true，失败返回 false
     */
    virtual bool unpack(const fs::path& packagePath,
                       const fs::path& outputDir,
                       std::function<void(int, int)> progressCallback = nullptr,
                       const std::string& password = "",
                       int* outErrorCode = nullptr) = 0;

    /**
     * @brief 获取包文件信息
     * @param packagePath 包文件路径
     * @return PackageInfo 包文件信息
     */
    virtual PackageInfo getPackageInfo(const fs::path& packagePath) = 0;

    /**
     * @brief 验证包文件完整性
     * @param packagePath 包文件路径
     * @return bool 完整返回 true，损坏返回 false
     */
    virtual bool verifyPackage(const fs::path& packagePath) = 0;
};

/**
 * @brief 文件打包器实现类
 *
 * 打包文件格式：
 * - 头部（Header）：
 *   - Magic Number: 4 字节（"DPKG"）
 *   - Version: 4 字节（版本号）
 *   - File Count: 4 字节（文件数量）
 *   - Total Size: 8 字节（总大小）
 *   - Checksum: 8 字节（校验和）
 *   - Reserved: 4 字节（保留）
 * - 文件索引区（File Index）：
 *   - 每个文件索引条目：
 *     - Path Length: 4 字节
 *     - Path: 变长（文件路径）
 *     - File Size: 8 字节
 *     - Data Offset: 8 字节（在数据区的偏移）
 *     - Metadata: 创建时间、修改时间、属性等
 * - 数据区（Data Area）：
 *   - 文件内容连续存储
 */
class Packager : public IPackager {
public:
    Packager();
    ~Packager();

    /**
     * @brief 打包文件
     * @param files 要打包的文件列表
     * @param packagePath 包文件路径
     * @param progressCallback 进度回调函数（可选）
     * @return bool 成功返回 true，失败返回 false
     */
    bool pack(const std::vector<FileInfo>& files,
             const fs::path& packagePath,
             std::function<void(int, int)> progressCallback = nullptr,
             const fs::path& basePath = "",
             IEncryptionStrategy* encryptor = nullptr,
             const std::string& password = "",
             class ICompressionStrategy* compressor = nullptr) override;

    /**
     * @brief 解包文件
     * @param packagePath 包文件路径
     * @param outputDir 输出目录
     * @param progressCallback 进度回调函数（可选）
     * @param password 解密密码（可选）
     * @param outErrorCode 输出错误码（可选）:0=成功;1=密码错误;2=IO/格式错误
     * @return bool 成功返回 true，失败返回 false
     */
    bool unpack(const fs::path& packagePath,
               const fs::path& outputDir,
               std::function<void(int, int)> progressCallback = nullptr,
               const std::string& password = "",
               int* outErrorCode = nullptr) override;

    /**
     * @brief 获取包文件信息
     * @param packagePath 包文件路径
     * @return PackageInfo 包文件信息
     */
    PackageInfo getPackageInfo(const fs::path& packagePath) override;

    /**
     * @brief 验证包文件完整性
     * @param packagePath 包文件路径
     * @return bool 完整返回 true，损坏返回 false
     */
    bool verifyPackage(const fs::path& packagePath) override;

private:
    static constexpr const char* MAGIC_NUMBER = "DPKG";
    static constexpr uint32_t VERSION_V1 = 1;    // 无加密无压缩
    static constexpr uint32_t VERSION_V2 = 2;    // 支持加密
    static constexpr uint32_t VERSION_V3 = 3;    // 支持加密+压缩
    static constexpr uint32_t VERSION = VERSION_V3;
    static constexpr size_t HEADER_SIZE_V1 = 32;
    // V2 头部:V1(32) + encryptionFlag(4) + reserved(4) + verifyBlockSize(4) = 44
    static constexpr size_t HEADER_SIZE_V2 = 44;
    // V3 头部:V2(44) + compressionCode(4) = 48
    static constexpr size_t HEADER_SIZE_V3 = 48;
    static constexpr const char* PASSWORD_MAGIC = "DPKG_PWD_CHECK_v2";

    // 压缩算法编码
    static constexpr uint32_t COMPRESSION_NONE = 0;
    static constexpr uint32_t COMPRESSION_HUFFMAN = 1;
    static constexpr uint32_t COMPRESSION_ZLIB = 2;

    /**
     * @brief 写入包文件头部(V3:加密标志、密码验证块、压缩编码)
     */
    bool writeHeader(std::ostream& file,
                     uint32_t fileCount,
                     uint64_t totalSize,
                     bool encrypted,
                     const std::vector<uint8_t>& verifyBlock,
                     uint32_t compressionCode);

    /**
     * @brief 读取包文件头部
     */
    bool readHeader(std::ifstream& file,
                    PackageInfo& info,
                    bool& outEncrypted,
                    std::vector<uint8_t>& outVerifyBlock,
                    uint64_t& outHeaderEndOffset,
                    uint32_t& outCompressionCode);

    /**
     * @brief 写入文件索引
     * @param file 输出文件流
     * @param files 文件列表
     * @param dataStartOffset 数据区起始偏移
     * @param basePath 基础路径（用于转换为相对路径）
     * @return bool 成功返回 true
     */
    bool writeIndex(std::ostream& file, const std::vector<FileInfo>& files, uint64_t dataStartOffset, const fs::path& basePath = "");

    /**
     * @brief 读取文件索引
     * @param file 输入文件流
     * @param fileCount 文件数量
     * @param files 输出的文件列表
     * @return bool 成功返回 true
     */
    bool readIndex(std::ifstream& file, uint32_t fileCount, std::vector<FileInfo>& files);

    /**
     * @brief 写入文件数据
     * @param outputFile 输出文件流
     * @param files 文件列表
     * @param progressCallback 进度回调
     * @return bool 成功返回 true
     */
    bool writeData(std::ostream& outputFile,
                   const std::vector<FileInfo>& files,
                   std::function<void(int, int)> progressCallback,
                   IEncryptionStrategy* encryptor,
                   const std::string& password,
                   class ICompressionStrategy* compressor,
                   std::vector<uint64_t>& outStoredSizes);

    /**
     * @brief 构造密码验证块(加密固定明文)
     */
    std::vector<uint8_t> buildVerifyBlock(IEncryptionStrategy* encryptor,
                                          const std::string& password);

    /**
     * @brief 校验密码(解密验证块并比对固定明文)
     */
    bool checkPassword(IEncryptionStrategy* encryptor,
                       const std::vector<uint8_t>& verifyBlock,
                       const std::string& password);

    /**
     * @brief 计算校验和
     * @param data 数据
     * @param size 数据大小
     * @return uint64_t 校验和
     */
    uint64_t calculateChecksum(const char* data, size_t size);
};

#endif // PACKAGER_H