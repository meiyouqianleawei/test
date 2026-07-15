#include "ZlibCompression.h"
#include <stdexcept>
#include <algorithm>

// Zlib库头文件
#include <zlib.h>

ZlibCompression::ZlibCompression(int compressionLevel)
    : compressionLevel_(compressionLevel) {
    if (!isValidCompressionLevel(compressionLevel)) {
        compressionLevel_ = Z_DEFAULT_COMPRESSION;
    }
}

bool ZlibCompression::compress(const std::vector<uint8_t>& input,
                                 std::vector<uint8_t>& output) {
    if (input.empty()) {
        output.clear();
        return true;
    }

    // 初始化压缩流
    z_stream stream;
    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = Z_NULL;

    // 初始化压缩（deflate）
    int result = deflateInit(&stream, compressionLevel_);
    if (result != Z_OK) {
        return false;
    }

    // 设置输入数据
    stream.next_in = const_cast<uint8_t*>(input.data());
    stream.avail_in = static_cast<uInt>(input.size());

    // 估算压缩后大小（通常比原数据小，但最坏情况可能稍大）
    size_t maxOutputSize = compressBound(input.size());
    output.resize(maxOutputSize);

    stream.next_out = output.data();
    stream.avail_out = static_cast<uInt>(output.size());

    // 执行压缩
    result = deflate(&stream, Z_FINISH);

    if (result != Z_STREAM_END) {
        deflateEnd(&stream);
        output.clear();
        return false;
    }

    // 调整输出大小为实际大小
    output.resize(stream.total_out);

    // 清理压缩流
    deflateEnd(&stream);

    return true;
}

bool ZlibCompression::decompress(const std::vector<uint8_t>& input,
                                   std::vector<uint8_t>& output) {
    if (input.empty()) {
        output.clear();
        return true;
    }

    // 初始化解压流
    z_stream stream;
    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = Z_NULL;

    // 初始化解压（inflate）
    int result = inflateInit(&stream);
    if (result != Z_OK) {
        return false;
    }

    // 设置输入数据
    stream.next_in = const_cast<uint8_t*>(input.data());
    stream.avail_in = static_cast<uInt>(input.size());

    // 输出缓冲区（分块解压）
    output.clear();
    const size_t chunkSize = 16384;  // 16KB chunks
    std::vector<uint8_t> buffer(chunkSize);

    do {
        stream.next_out = buffer.data();
        stream.avail_out = static_cast<uInt>(chunkSize);

        result = inflate(&stream, Z_NO_FLUSH);

        if (result == Z_STREAM_ERROR || result == Z_DATA_ERROR || result == Z_MEM_ERROR) {
            inflateEnd(&stream);
            output.clear();
            return false;
        }

        // 追加解压后的数据
        size_t have = chunkSize - stream.avail_out;
        output.insert(output.end(), buffer.begin(), buffer.begin() + have);

    } while (stream.avail_out == 0);

    // 清理解压流
    inflateEnd(&stream);

    return result == Z_STREAM_END;
}

std::string ZlibCompression::getName() const {
    return "Zlib";
}

void ZlibCompression::setCompressionLevel(int level) {
    if (isValidCompressionLevel(level)) {
        compressionLevel_ = level;
    }
}

int ZlibCompression::getCompressionLevel() const {
    return compressionLevel_;
}

bool ZlibCompression::isValidCompressionLevel(int level) const {
    return (level >= 0 && level <= 9) || level == Z_DEFAULT_COMPRESSION;
}