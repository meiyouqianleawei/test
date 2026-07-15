#include "Packager.h"
#include "../encryption/XOREncryption.h"
#include "../compression/ICompressionStrategy.h"
#include "../compression/HuffmanCompression.h"
#include "../compression/ZlibCompression.h"
#include "../core/MetadataManager.h"
#include <fstream>
#include <cstring>
#include <algorithm>
#include <thread>
#include <chrono>
#include <memory>
#include <system_error>
#ifdef _WIN32
#include <windows.h>
#endif

/**
 * @brief 构造函数
 */
Packager::Packager() {
}

/**
 * @brief 析构函数
 */
Packager::~Packager() {
}

/**
 * @brief 打包文件
 * @param files 要打包的文件列表
 * @param packagePath 包文件路径
 * @param progressCallback 进度回调函数（可选）
 * @return bool 成功返回 true，失败返回 false
 */
bool Packager::pack(const std::vector<FileInfo>& files,
                   const fs::path& packagePath,
                   std::function<void(int, int)> progressCallback,
                   const fs::path& basePath,
                   IEncryptionStrategy* encryptor,
                   const std::string& password,
                   ICompressionStrategy* compressor) {
    if (files.empty()) {
        return false;
    }

    const bool encrypted = (encryptor != nullptr) && !password.empty();

    // 压缩算法编码
    uint32_t compressionCode = COMPRESSION_NONE;
    if (compressor) {
        std::string n = compressor->getName();
        if (n == "Huffman") compressionCode = COMPRESSION_HUFFMAN;
        else if (n == "Zlib") compressionCode = COMPRESSION_ZLIB;
        else compressionCode = COMPRESSION_NONE;
    }

    try {
        // 原始文件字节总和(用于头部 totalSize)
        uint64_t totalSize = 0;
        for (const auto& fileInfo : files) {
            if (fs::exists(fileInfo.path) && !fs::is_directory(fileInfo.path)) {
                totalSize += fs::file_size(fileInfo.path);
            }
        }

        // 密码验证块
        std::vector<uint8_t> verifyBlock;
        if (encrypted) {
            verifyBlock = buildVerifyBlock(encryptor, password);
            if (verifyBlock.empty()) return false;
        }

        // 若目标已存在,先移除只读属性并删除,避免第二次备份到同一路径时打开失败
        if (fs::exists(packagePath)) {
            std::error_code ec;
#ifdef _WIN32
            DWORD attrs = GetFileAttributesW(packagePath.wstring().c_str());
            if (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_READONLY)) {
                SetFileAttributesW(packagePath.wstring().c_str(), attrs & ~FILE_ATTRIBUTE_READONLY);
            }
#endif
            fs::remove(packagePath, ec);
            if (ec) {
                // 删除失败(可能被其他进程锁定),放弃
                return false;
            }
        }

        // 确保父目录存在
        if (packagePath.has_parent_path()) {
            std::error_code ec;
            fs::create_directories(packagePath.parent_path(), ec);
        }

        // 使用可读写的 fstream,方便数据写完后 seek 回索引区回写
        std::fstream outputFile(packagePath,
            std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc);
        if (!outputFile.is_open()) return false;

        // 写头部
        if (!writeHeader(outputFile,
                         static_cast<uint32_t>(files.size()),
                         totalSize,
                         encrypted,
                         verifyBlock,
                         compressionCode)) {
            outputFile.close();
            return false;
        }

        // 记录索引区起点 = 头部大小(V3) + 验证块
        const uint64_t indexStartOffset = HEADER_SIZE_V3 + verifyBlock.size();

        // 计算索引区大小(此刻还不知道每文件真实 storedSize,索引先写占位,数据写完再回写)
        uint64_t indexSize = 0;
        for (const auto& fileInfo : files) {
            fs::path storedPath = fileInfo.path;
            if (!basePath.empty() && storedPath.is_absolute()) {
                storedPath = fs::relative(fileInfo.path, basePath);
            }
            // 每条:pathLen(4) + path + storedSize(8) + offset(8) + createT(8) + writeT(8) + attr(4)
            indexSize += 4 + storedPath.string().size() + 8 + 8 + 8 + 8 + 4;
        }
        const uint64_t dataStartOffset = indexStartOffset + indexSize;

        // 先占位写索引(用 0 填充),稍后回写
        std::vector<char> zeros(static_cast<size_t>(indexSize), 0);
        outputFile.write(zeros.data(), static_cast<std::streamsize>(indexSize));

        // 定位到数据区起点
        outputFile.seekp(static_cast<std::streamoff>(dataStartOffset));

        // 写数据,返回每个文件在数据区中占用的字节数
        std::vector<uint64_t> storedSizes;
        if (!writeData(outputFile, files, progressCallback,
                       encrypted ? encryptor : nullptr,
                       encrypted ? password : std::string(),
                       compressor,
                       storedSizes)) {
            outputFile.close();
            return false;
        }

        // 回到索引位置,写入真实索引
        outputFile.seekp(static_cast<std::streamoff>(indexStartOffset));

        uint64_t currentOffset = dataStartOffset;
        for (size_t i = 0; i < files.size(); ++i) {
            const auto& fileInfo = files[i];
            fs::path storedPath = fileInfo.path;
            if (!basePath.empty() && storedPath.is_absolute()) {
                storedPath = fs::relative(fileInfo.path, basePath);
            }
            std::string pathStr = storedPath.string();
            uint32_t pathLength = static_cast<uint32_t>(pathStr.size());
            outputFile.write(reinterpret_cast<const char*>(&pathLength), sizeof(pathLength));
            outputFile.write(pathStr.c_str(), pathLength);

            uint64_t stored = storedSizes[i];
            outputFile.write(reinterpret_cast<const char*>(&stored), sizeof(stored));
            outputFile.write(reinterpret_cast<const char*>(&currentOffset), sizeof(currentOffset));

            outputFile.write(reinterpret_cast<const char*>(&fileInfo.creationTime), sizeof(fileInfo.creationTime));
            outputFile.write(reinterpret_cast<const char*>(&fileInfo.lastWriteTime), sizeof(fileInfo.lastWriteTime));
            outputFile.write(reinterpret_cast<const char*>(&fileInfo.attributes), sizeof(fileInfo.attributes));

            currentOffset += stored;
        }

        outputFile.close();
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

/**
 * @brief 解包文件
 * @param packagePath 包文件路径
 * @param outputDir 输出目录
 * @param progressCallback 进度回调函数（可选）
 * @return bool 成功返回 true，失败返回 false
 */
bool Packager::unpack(const fs::path& packagePath,
                     const fs::path& outputDir,
                     std::function<void(int, int)> progressCallback,
                     const std::string& password,
                     int* outErrorCode) {
    auto setErr = [&](int code) { if (outErrorCode) *outErrorCode = code; };
    setErr(0);

    if (!fs::exists(packagePath)) {
        setErr(2);
        return false;
    }

    try {
        std::ifstream inputFile(packagePath, std::ios::binary);
        if (!inputFile.is_open()) {
            setErr(2);
            return false;
        }

        // 读取头部(含加密标志、验证块、压缩编码)
        PackageInfo packageInfo;
        bool encrypted = false;
        std::vector<uint8_t> verifyBlock;
        uint64_t headerEnd = 0;
        uint32_t compressionCode = COMPRESSION_NONE;
        if (!readHeader(inputFile, packageInfo, encrypted, verifyBlock, headerEnd, compressionCode)) {
            inputFile.close();
            setErr(2);
            return false;
        }

        // 构造解压器
        std::unique_ptr<ICompressionStrategy> decompressor;
        if (compressionCode == COMPRESSION_HUFFMAN) {
            decompressor = std::make_unique<DataBackup::HuffmanCompression>();
        } else if (compressionCode == COMPRESSION_ZLIB) {
            decompressor = std::make_unique<ZlibCompression>();
        }

        if (packageInfo.version.empty()) {
            inputFile.close();
            setErr(2);
            return false;
        }

        // 若加密:必须提供密码并通过校验
        std::unique_ptr<IEncryptionStrategy> decryptor;
        if (encrypted) {
            if (password.empty()) {
                inputFile.close();
                setErr(1);
                return false;
            }
            decryptor = std::make_unique<DataBackup::XOREncryption>();
            if (!checkPassword(decryptor.get(), verifyBlock, password)) {
                inputFile.close();
                setErr(1);
                return false;
            }
        }

        // 读取文件索引
        std::vector<FileInfo> files;
        if (!readIndex(inputFile, packageInfo.fileCount, files)) {
            inputFile.close();
            setErr(2);
            return false;
        }

        if (!fs::exists(outputDir)) {
            fs::create_directories(outputDir);
        }

        // 重新定位到索引起始:头部之后
        inputFile.clear();
        inputFile.seekg(static_cast<std::streamoff>(headerEnd));

        struct FileIndexInfo {
            std::string path;
            uint64_t size;
            uint64_t dataOffset;
            uint64_t creationTime;
            uint64_t lastWriteTime;
            uint32_t attributes;
        };

        std::vector<FileIndexInfo> indexInfos;

        for (uint32_t i = 0; i < packageInfo.fileCount; i++) {
            // 读取路径长度
            uint32_t pathLength;
            inputFile.read(reinterpret_cast<char*>(&pathLength), sizeof(pathLength));

            // 读取路径
            std::vector<char> pathBuffer(pathLength);
            inputFile.read(pathBuffer.data(), pathLength);
            std::string pathStr(pathBuffer.data(), pathLength);

            // 读取文件大小
            uint64_t fileSize;
            inputFile.read(reinterpret_cast<char*>(&fileSize), sizeof(fileSize));

            // 读取数据偏移（关键：这是文件数据的实际位置）
            uint64_t dataOffset;
            inputFile.read(reinterpret_cast<char*>(&dataOffset), sizeof(dataOffset));

            // 读取元数据
            uint64_t creationTime, lastWriteTime;
            uint32_t attributes;
            inputFile.read(reinterpret_cast<char*>(&creationTime), sizeof(creationTime));
            inputFile.read(reinterpret_cast<char*>(&lastWriteTime), sizeof(lastWriteTime));
            inputFile.read(reinterpret_cast<char*>(&attributes), sizeof(attributes));

            // 保存索引信息（包括数据偏移）
            FileIndexInfo info;
            info.path = pathStr;
            info.size = fileSize;
            info.dataOffset = dataOffset;  // 使用存储的偏移
            info.creationTime = creationTime;
            info.lastWriteTime = lastWriteTime;
            info.attributes = attributes;
            indexInfos.push_back(info);
        }

        // 按索引解包
        MetadataManager metaManager;
        for (size_t i = 0; i < indexInfos.size(); i++) {
            const auto& info = indexInfos[i];

            fs::path inputPath(info.path);
            fs::path outputPath;
            if (inputPath.is_absolute()) {
                outputPath = outputDir / inputPath.filename();
            } else {
                outputPath = outputDir / inputPath;
            }

            fs::path parentDir = outputPath.parent_path();
            if (!parentDir.empty() && !fs::exists(parentDir)) {
                fs::create_directories(parentDir);
            }

            inputFile.seekg(static_cast<std::streamoff>(info.dataOffset));

            std::vector<uint8_t> stage(info.size);
            if (info.size > 0) {
                inputFile.read(reinterpret_cast<char*>(stage.data()),
                               static_cast<std::streamsize>(info.size));
            }

            // 解密(先) → 解压(后),顺序与 pack 相反
            if (encrypted && !stage.empty()) {
                std::vector<uint8_t> plain;
                if (!decryptor->decrypt(stage, plain, password)) {
                    inputFile.close();
                    setErr(1);
                    return false;
                }
                stage = std::move(plain);
            }

            if (decompressor && !stage.empty()) {
                std::vector<uint8_t> raw;
                if (!decompressor->decompress(stage, raw)) {
                    inputFile.close();
                    setErr(2);
                    return false;
                }
                stage = std::move(raw);
            }

            std::ofstream outputFile(outputPath, std::ios::binary);
            if (!outputFile.is_open()) {
                continue;
            }
            if (!stage.empty()) {
                outputFile.write(reinterpret_cast<const char*>(stage.data()),
                                 static_cast<std::streamsize>(stage.size()));
            }
            outputFile.close();

            // 恢复元数据(时间戳 + 属性)
            FileInfo fi;
            fi.path = outputPath;
            fi.creationTime = info.creationTime;
            fi.lastWriteTime = info.lastWriteTime;
            fi.lastAccessTime = info.lastWriteTime;
            fi.attributes = info.attributes;
            metaManager.setMetadata(fi, outputPath);

            if (progressCallback) {
                progressCallback(static_cast<int>(i + 1), static_cast<int>(indexInfos.size()));
            }
        }

        inputFile.close();
        return true;
    } catch (const std::exception& e) {
        setErr(2);
        return false;
    }
}

/**
 * @brief 获取包文件信息
 * @param packagePath 包文件路径
 * @return PackageInfo 包文件信息
 */
PackageInfo Packager::getPackageInfo(const fs::path& packagePath) {
    PackageInfo info;

    if (!fs::exists(packagePath)) {
        return info;
    }

    try {
        std::ifstream file(packagePath, std::ios::binary);
        if (!file.is_open()) {
            return info;
        }

        bool encrypted = false;
        std::vector<uint8_t> verifyBlock;
        uint64_t headerEnd = 0;
        uint32_t compressionCode = COMPRESSION_NONE;
        readHeader(file, info, encrypted, verifyBlock, headerEnd, compressionCode);
        file.close();
    } catch (const std::exception& e) {
        // 返回空信息
    }

    return info;
}

/**
 * @brief 验证包文件完整性
 * @param packagePath 包文件路径
 * @return bool 完整返回 true，损坏返回 false
 */
bool Packager::verifyPackage(const fs::path& packagePath) {
    if (!fs::exists(packagePath)) {
        return false;
    }

    try {
        std::ifstream file(packagePath, std::ios::binary);
        if (!file.is_open()) {
            return false;
        }

        // 读取 Magic Number
        char magic[4];
        file.read(magic, 4);

        if (std::strncmp(magic, MAGIC_NUMBER, 4) != 0) {
            file.close();
            return false;
        }

        // 读取版本号
        uint32_t version;
        file.read(reinterpret_cast<char*>(&version), sizeof(version));

        if (version != VERSION_V1 && version != VERSION_V2 && version != VERSION_V3) {
            file.close();
            return false;
        }

        // 读取文件数量
        uint32_t fileCount;
        file.read(reinterpret_cast<char*>(&fileCount), sizeof(fileCount));

        if (fileCount == 0) {
            file.close();
            return false;
        }

        file.close();
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

/**
 * @brief 写入包文件头部
 * @param file 输出文件流
 * @param fileCount 文件数量
 * @param totalSize 总大小
 * @return bool 成功返回 true
 */
bool Packager::writeHeader(std::ostream& file,
                           uint32_t fileCount,
                           uint64_t totalSize,
                           bool encrypted,
                           const std::vector<uint8_t>& verifyBlock,
                           uint32_t compressionCode) {
    try {
        // Magic Number(4)
        file.write(MAGIC_NUMBER, 4);

        // Version(4) —— 始终写 V3
        uint32_t version = VERSION_V3;
        file.write(reinterpret_cast<const char*>(&version), sizeof(version));

        // File Count(4)
        file.write(reinterpret_cast<const char*>(&fileCount), sizeof(fileCount));

        // Total Size(8)
        file.write(reinterpret_cast<const char*>(&totalSize), sizeof(totalSize));

        // Checksum(8) —— 保留
        uint64_t checksum = 0;
        file.write(reinterpret_cast<const char*>(&checksum), sizeof(checksum));

        // Reserved(4) —— V1 剩余位,V2 保留
        uint32_t reserved = 0;
        file.write(reinterpret_cast<const char*>(&reserved), sizeof(reserved));

        // V2 扩展:encryptionFlag(4) + reserved2(4) + verifyBlockSize(4)
        uint32_t encFlag = encrypted ? 1u : 0u;
        file.write(reinterpret_cast<const char*>(&encFlag), sizeof(encFlag));

        uint32_t reserved2 = 0;
        file.write(reinterpret_cast<const char*>(&reserved2), sizeof(reserved2));

        uint32_t verifyBlockSize = static_cast<uint32_t>(verifyBlock.size());
        file.write(reinterpret_cast<const char*>(&verifyBlockSize), sizeof(verifyBlockSize));

        // V3 扩展:compressionCode(4)
        file.write(reinterpret_cast<const char*>(&compressionCode), sizeof(compressionCode));

        // 写入密码验证块(仅加密包)
        if (verifyBlockSize > 0) {
            file.write(reinterpret_cast<const char*>(verifyBlock.data()), verifyBlockSize);
        }

        return file.good();
    } catch (const std::exception& e) {
        return false;
    }
}

/**
 * @brief 读取包文件头部
 * @param file 输入文件流
 * @param info 输出的包信息
 * @return bool 成功返回 true
 */
bool Packager::readHeader(std::ifstream& file,
                          PackageInfo& info,
                          bool& outEncrypted,
                          std::vector<uint8_t>& outVerifyBlock,
                          uint64_t& outHeaderEndOffset,
                          uint32_t& outCompressionCode) {
    try {
        outEncrypted = false;
        outVerifyBlock.clear();
        outCompressionCode = COMPRESSION_NONE;

        char magic[4];
        file.read(magic, 4);
        if (std::strncmp(magic, MAGIC_NUMBER, 4) != 0) return false;

        uint32_t version = 0;
        file.read(reinterpret_cast<char*>(&version), sizeof(version));
        info.version = std::to_string(version);

        file.read(reinterpret_cast<char*>(&info.fileCount), sizeof(info.fileCount));
        file.read(reinterpret_cast<char*>(&info.totalSize), sizeof(info.totalSize));
        file.read(reinterpret_cast<char*>(&info.checksum), sizeof(info.checksum));

        uint32_t reserved = 0;
        file.read(reinterpret_cast<char*>(&reserved), sizeof(reserved));

        if (version == VERSION_V1) {
            outHeaderEndOffset = HEADER_SIZE_V1;
            return file.good();
        }
        if (version != VERSION_V2 && version != VERSION_V3) return false;

        // V2/V3 共有:encFlag + reserved2 + verifyBlockSize
        uint32_t encFlag = 0;
        file.read(reinterpret_cast<char*>(&encFlag), sizeof(encFlag));
        uint32_t reserved2 = 0;
        file.read(reinterpret_cast<char*>(&reserved2), sizeof(reserved2));
        uint32_t verifyBlockSize = 0;
        file.read(reinterpret_cast<char*>(&verifyBlockSize), sizeof(verifyBlockSize));

        outEncrypted = (encFlag != 0);

        // V3 额外:compressionCode
        size_t headerFixed = HEADER_SIZE_V2;
        if (version == VERSION_V3) {
            file.read(reinterpret_cast<char*>(&outCompressionCode), sizeof(outCompressionCode));
            headerFixed = HEADER_SIZE_V3;
        }

        if (verifyBlockSize > 0) {
            outVerifyBlock.resize(verifyBlockSize);
            file.read(reinterpret_cast<char*>(outVerifyBlock.data()), verifyBlockSize);
        }

        outHeaderEndOffset = headerFixed + verifyBlockSize;
        return file.good();
    } catch (const std::exception& e) {
        return false;
    }
}

/**
 * @brief 写入文件索引
 * @param file 输出文件流
 * @param files 文件列表
 * @param dataStartOffset 数据区起始偏移
 * @return bool 成功返回 true
 */
bool Packager::writeIndex(std::ostream& file, const std::vector<FileInfo>& files, uint64_t dataStartOffset, const fs::path& basePath) {
    try {
        // 注:XOR 加密不改变字节数,所以存储大小 = 原始大小
        // 此处保持"存储大小"含义:数据区中该文件占用的字节数
        uint64_t currentOffset = dataStartOffset;

        for (const auto& fileInfo : files) {
            fs::path storedPath = fileInfo.path;
            if (!basePath.empty() && storedPath.is_absolute()) {
                storedPath = fs::relative(fileInfo.path, basePath);
            }

            std::string pathStr = storedPath.string();
            uint32_t pathLength = static_cast<uint32_t>(pathStr.size());
            file.write(reinterpret_cast<const char*>(&pathLength), sizeof(pathLength));
            file.write(pathStr.c_str(), pathLength);

            uint64_t fileSize = 0;
            if (fs::exists(fileInfo.path) && !fs::is_directory(fileInfo.path)) {
                fileSize = fs::file_size(fileInfo.path);
            }
            file.write(reinterpret_cast<const char*>(&fileSize), sizeof(fileSize));

            file.write(reinterpret_cast<const char*>(&currentOffset), sizeof(currentOffset));

            file.write(reinterpret_cast<const char*>(&fileInfo.creationTime), sizeof(fileInfo.creationTime));
            file.write(reinterpret_cast<const char*>(&fileInfo.lastWriteTime), sizeof(fileInfo.lastWriteTime));
            file.write(reinterpret_cast<const char*>(&fileInfo.attributes), sizeof(fileInfo.attributes));

            currentOffset += fileSize;
        }

        return file.good();
    } catch (const std::exception& e) {
        return false;
    }
}

/**
 * @brief 读取文件索引
 * @param file 输入文件流
 * @param fileCount 文件数量
 * @param files 输出的文件列表
 * @return bool 成功返回 true
 */
bool Packager::readIndex(std::ifstream& file, uint32_t fileCount, std::vector<FileInfo>& files) {
    try {
        files.clear();
        files.reserve(fileCount);

        for (uint32_t i = 0; i < fileCount; i++) {
            // 读取路径长度
            uint32_t pathLength;
            file.read(reinterpret_cast<char*>(&pathLength), sizeof(pathLength));

            // 读取路径
            std::vector<char> pathBuffer(pathLength);
            file.read(pathBuffer.data(), pathLength);
            std::string pathStr(pathBuffer.data(), pathLength);

            // 读取文件大小
            uint64_t fileSize;
            file.read(reinterpret_cast<char*>(&fileSize), sizeof(fileSize));

            // 读取数据偏移（暂时忽略）
            uint64_t dataOffset;
            file.read(reinterpret_cast<char*>(&dataOffset), sizeof(dataOffset));

            // 创建 FileInfo
            FileInfo fileInfo{fs::path(pathStr)};
            fileInfo.size = fileSize;

            // 读取元数据
            file.read(reinterpret_cast<char*>(&fileInfo.creationTime), sizeof(fileInfo.creationTime));
            file.read(reinterpret_cast<char*>(&fileInfo.lastWriteTime), sizeof(fileInfo.lastWriteTime));
            file.read(reinterpret_cast<char*>(&fileInfo.attributes), sizeof(fileInfo.attributes));

            files.push_back(fileInfo);
        }

        return file.good();
    } catch (const std::exception& e) {
        return false;
    }
}

/**
 * @brief 写入文件数据
 * @param outputFile 输出文件流
 * @param files 文件列表
 * @param progressCallback 进度回调
 * @return bool 成功返回 true
 */
bool Packager::writeData(std::ostream& outputFile,
                         const std::vector<FileInfo>& files,
                         std::function<void(int, int)> progressCallback,
                         IEncryptionStrategy* encryptor,
                         const std::string& password,
                         ICompressionStrategy* compressor,
                         std::vector<uint64_t>& outStoredSizes) {
    try {
        outStoredSizes.clear();
        outStoredSizes.reserve(files.size());

        const bool doEncrypt = (encryptor != nullptr) && !password.empty();
        const bool doCompress = (compressor != nullptr);

        for (size_t i = 0; i < files.size(); i++) {
            const auto& fileInfo = files[i];
            uint64_t stored = 0;

            if (!fs::exists(fileInfo.path) || fs::is_directory(fileInfo.path)) {
                outStoredSizes.push_back(stored);
                if (progressCallback) {
                    progressCallback(static_cast<int>(i + 1), static_cast<int>(files.size()));
                }
                continue;
            }

            // 读整个文件到内存(压缩必须整块进行,分块压缩需要每块头部)
            std::ifstream inputFile(fileInfo.path, std::ios::binary);
            if (!inputFile.is_open()) {
                outStoredSizes.push_back(stored);
                continue;
            }
            inputFile.seekg(0, std::ios::end);
            std::streamsize fileSize = inputFile.tellg();
            inputFile.seekg(0, std::ios::beg);

            std::vector<uint8_t> plain(static_cast<size_t>(fileSize));
            if (fileSize > 0) {
                inputFile.read(reinterpret_cast<char*>(plain.data()), fileSize);
            }
            inputFile.close();

            // 压缩(先) → 加密(后)
            std::vector<uint8_t> stage = std::move(plain);

            if (doCompress && !stage.empty()) {
                std::vector<uint8_t> compressed;
                if (!compressor->compress(stage, compressed)) {
                    return false;
                }
                stage = std::move(compressed);
            }

            if (doEncrypt && !stage.empty()) {
                std::vector<uint8_t> cipher;
                if (!encryptor->encrypt(stage, cipher, password)) {
                    return false;
                }
                stage = std::move(cipher);
            }

            if (!stage.empty()) {
                outputFile.write(reinterpret_cast<const char*>(stage.data()),
                                 static_cast<std::streamsize>(stage.size()));
                stored = stage.size();
            }

            outStoredSizes.push_back(stored);

            if (progressCallback) {
                progressCallback(static_cast<int>(i + 1), static_cast<int>(files.size()));
            }
        }

        return outputFile.good();
    } catch (const std::exception& e) {
        return false;
    }
}

std::vector<uint8_t> Packager::buildVerifyBlock(IEncryptionStrategy* encryptor,
                                                const std::string& password) {
    std::vector<uint8_t> result;
    if (!encryptor) return result;

    std::string magic = PASSWORD_MAGIC;
    std::vector<uint8_t> plain(magic.begin(), magic.end());
    if (!encryptor->encrypt(plain, result, password)) {
        result.clear();
    }
    return result;
}

bool Packager::checkPassword(IEncryptionStrategy* encryptor,
                             const std::vector<uint8_t>& verifyBlock,
                             const std::string& password) {
    if (!encryptor || verifyBlock.empty() || password.empty()) {
        return false;
    }
    std::vector<uint8_t> plain;
    if (!encryptor->decrypt(verifyBlock, plain, password)) {
        return false;
    }
    std::string magic = PASSWORD_MAGIC;
    if (plain.size() != magic.size()) {
        return false;
    }
    return std::memcmp(plain.data(), magic.data(), magic.size()) == 0;
}

/**
 * @brief 计算校验和
 * @param data 数据
 * @param size 数据大小
 * @return uint64_t 校验和
 */
uint64_t Packager::calculateChecksum(const char* data, size_t size) {
    uint64_t checksum = 0;

    for (size_t i = 0; i < size; i++) {
        checksum += static_cast<uint64_t>(data[i]);
    }

    return checksum;
}