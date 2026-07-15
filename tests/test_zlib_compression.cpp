// Zlib压缩模块测试 - 验证压缩解压功能
#include <gtest/gtest.h>
#include "compression/ZlibCompression.h"
#include <vector>
#include <string>
#include <random>
#include <algorithm>

class ZlibCompressionTest : public ::testing::Test {
protected:
    ZlibCompression compressor_;

    void SetUp() override {
        compressor_ = ZlibCompression();
    }

    // 生成随机数据
    std::vector<uint8_t> generateRandomData(size_t size) {
        std::vector<uint8_t> data(size);
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 255);
        for (auto& byte : data) {
            byte = static_cast<uint8_t>(dis(gen));
        }
        return data;
    }

    // 生成重复数据（高压缩率）
    std::vector<uint8_t> generateRepeatingData(size_t size, uint8_t value) {
        return std::vector<uint8_t>(size, value);
    }
};

// 测试1：空数据压缩解压
TEST_F(ZlibCompressionTest, EmptyData) {
    std::vector<uint8_t> input;
    std::vector<uint8_t> compressed;
    std::vector<uint8_t> decompressed;

    EXPECT_TRUE(compressor_.compress(input, compressed));
    EXPECT_TRUE(compressed.empty());

    EXPECT_TRUE(compressor_.decompress(compressed, decompressed));
    EXPECT_TRUE(decompressed.empty());
}

// 测试2：简单文本压缩解压
TEST_F(ZlibCompressionTest, SimpleText) {
    std::string text = "Hello, World! This is a test for Zlib compression.";
    std::vector<uint8_t> input(text.begin(), text.end());

    std::vector<uint8_t> compressed;
    std::vector<uint8_t> decompressed;

    // 压缩
    EXPECT_TRUE(compressor_.compress(input, compressed));
    EXPECT_FALSE(compressed.empty());

    // 解压
    EXPECT_TRUE(compressor_.decompress(compressed, decompressed));
    EXPECT_EQ(input, decompressed);
}

// 测试3：重复数据压缩（应显著减小）
TEST_F(ZlibCompressionTest, RepeatingDataCompression) {
    size_t size = 10000;
    auto input = generateRepeatingData(size, 'A');

    std::vector<uint8_t> compressed;
    std::vector<uint8_t> decompressed;

    EXPECT_TRUE(compressor_.compress(input, compressed));

    // 压缩后应该显著变小（重复数据压缩率高）
    EXPECT_LT(compressed.size(), input.size() / 10);

    // 解压验证
    EXPECT_TRUE(compressor_.decompress(compressed, decompressed));
    EXPECT_EQ(input, decompressed);
}

// 测试4：随机数据压缩（压缩率低）
TEST_F(ZlibCompressionTest, RandomDataCompression) {
    size_t size = 10000;
    auto input = generateRandomData(size);

    std::vector<uint8_t> compressed;
    std::vector<uint8_t> decompressed;

    EXPECT_TRUE(compressor_.compress(input, compressed));

    // 随机数据压缩率低，可能略大于原数据
    EXPECT_LE(compressed.size(), input.size() * 2);

    // 解压验证
    EXPECT_TRUE(compressor_.decompress(compressed, decompressed));
    EXPECT_EQ(input, decompressed);
}

// 测试5：不同压缩级别
TEST_F(ZlibCompressionTest, DifferentCompressionLevels) {
    std::string text(1000, 'X');  // 1000个'X'
    std::vector<uint8_t> input(text.begin(), text.end());

    std::vector<size_t> compressedSizes;

    // 测试不同压缩级别
    for (int level = 1; level <= 9; level++) {
        ZlibCompression compressor(level);
        std::vector<uint8_t> compressed;
        std::vector<uint8_t> decompressed;

        EXPECT_TRUE(compressor.compress(input, compressed));
        compressedSizes.push_back(compressed.size());

        EXPECT_TRUE(compressor.decompress(compressed, decompressed));
        EXPECT_EQ(input, decompressed);
    }

    // 更高压缩级别应该产生更小的输出（通常）
    EXPECT_LE(compressedSizes.back(), compressedSizes.front());
}

// 测试6：大文件压缩
TEST_F(ZlibCompressionTest, LargeFileCompression) {
    // 生成1MB数据
    size_t size = 1024 * 1024;
    auto input = generateRepeatingData(size, 'Z');

    std::vector<uint8_t> compressed;
    std::vector<uint8_t> decompressed;

    EXPECT_TRUE(compressor_.compress(input, compressed));
    EXPECT_LT(compressed.size(), input.size());

    EXPECT_TRUE(compressor_.decompress(compressed, decompressed));
    EXPECT_EQ(input.size(), decompressed.size());
    EXPECT_EQ(input, decompressed);
}

// 测试7：无效数据解压
TEST_F(ZlibCompressionTest, InvalidDataDecompression) {
    std::vector<uint8_t> invalidData = {1, 2, 3, 4, 5};  // 非压缩数据
    std::vector<uint8_t> output;

    // 应该失败或返回空
    bool result = compressor_.decompress(invalidData, output);
    EXPECT_FALSE(result);
}

// 测试8：压缩后再次压缩（幂等性测试）
TEST_F(ZlibCompressionTest, DoubleCompression) {
    // 使用足够大的数据以确保压缩效果明显
    std::string text = "This is a longer test string for double compression testing. "
                       "We need enough data to see compression benefits. "
                       "The zlib header overhead is about 10+ bytes, so small data may grow. "
                       "This longer text should definitely compress smaller.";
    std::vector<uint8_t> input(text.begin(), text.end());

    std::vector<uint8_t> compressed1;
    std::vector<uint8_t> compressed2;

    EXPECT_TRUE(compressor_.compress(input, compressed1));
    EXPECT_TRUE(compressor_.compress(compressed1, compressed2));

    // 第一次压缩应该减小大小（对于足够大的数据）
    EXPECT_LT(compressed1.size(), input.size());

    // 第二次压缩通常不会进一步减小
    EXPECT_GE(compressed2.size(), compressed1.size());
}

// 测试9：获取压缩器名称
TEST_F(ZlibCompressionTest, GetName) {
    EXPECT_EQ(compressor_.getName(), "Zlib");
}

// 测试10：设置和获取压缩级别
TEST_F(ZlibCompressionTest, CompressionLevelOperations) {
    // 默认级别
    EXPECT_EQ(compressor_.getCompressionLevel(), -1);

    // 设置有效级别
    compressor_.setCompressionLevel(5);
    EXPECT_EQ(compressor_.getCompressionLevel(), 5);

    // 设置无效级别应被忽略
    compressor_.setCompressionLevel(100);
    EXPECT_EQ(compressor_.getCompressionLevel(), 5);
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}