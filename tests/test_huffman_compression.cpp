// Huffman压缩模块测试 - 验证压缩解压功能

#include <gtest/gtest.h>
#include "compression/HuffmanCompression.h"
#include <vector>
#include <random>
#include <algorithm>

using namespace DataBackup;

// 测试类
class HuffmanCompressionTest : public ::testing::Test {
protected:
    HuffmanCompression compressor_;

    void SetUp() override {
        // 每个测试前的初始化
    }

    void TearDown() override {
        // 每个测试后的清理
    }

    // 辅助函数：生成随机数据
    std::vector<uint8_t> generateRandomData(size_t size) {
        std::vector<uint8_t> data(size);
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<int> dis(0, 255);  // 使用int类型

        for (size_t i = 0; i < size; i++) {
            data[i] = static_cast<uint8_t>(dis(gen));
        }
        return data;
    }

    // 辅助函数：生成重复数据（高压缩率）
    std::vector<uint8_t> generateRepeatingData(size_t size, uint8_t pattern) {
        std::vector<uint8_t> data(size, pattern);
        return data;
    }
};

// 测试1：空数据压缩解压
TEST_F(HuffmanCompressionTest, EmptyData) {
    std::vector<uint8_t> input;
    std::vector<uint8_t> compressed;
    std::vector<uint8_t> decompressed;

    EXPECT_TRUE(compressor_.compress(input, compressed));
    EXPECT_TRUE(compressor_.decompress(compressed, decompressed));
    EXPECT_TRUE(decompressed.empty());
}

// 测试2：简单文本压缩解压
TEST_F(HuffmanCompressionTest, SimpleText) {
    std::string text = "Hello, Huffman Compression!";
    std::vector<uint8_t> input(text.begin(), text.end());
    
    std::vector<uint8_t> compressed;
    std::vector<uint8_t> decompressed;

    EXPECT_TRUE(compressor_.compress(input, compressed));
    EXPECT_TRUE(compressor_.decompress(compressed, decompressed));
    EXPECT_EQ(input, decompressed);
}

// 测试3：单字节重复（边界情况）
TEST_F(HuffmanCompressionTest, SingleByte) {
    std::vector<uint8_t> input(100, 'A');  // 100个'A'
    
    std::vector<uint8_t> compressed;
    std::vector<uint8_t> decompressed;

    EXPECT_TRUE(compressor_.compress(input, compressed));
    EXPECT_TRUE(compressor_.decompress(compressed, decompressed));
    EXPECT_EQ(input, decompressed);
}

// 测试4：两种字节交替
TEST_F(HuffmanCompressionTest, TwoBytesAlternating) {
    std::string text = "ABABABABABABABABABABABABABABABAB";
    std::vector<uint8_t> input(text.begin(), text.end());
    
    std::vector<uint8_t> compressed;
    std::vector<uint8_t> decompressed;

    EXPECT_TRUE(compressor_.compress(input, compressed));
    EXPECT_TRUE(compressor_.decompress(compressed, decompressed));
    EXPECT_EQ(input, decompressed);
}

// 测试5：重复数据压缩（高压缩率）
TEST_F(HuffmanCompressionTest, RepeatingDataCompression) {
    // 生成10KB的重复数据
    std::vector<uint8_t> input = generateRepeatingData(10240, 'X');
    
    std::vector<uint8_t> compressed;
    std::vector<uint8_t> decompressed;

    EXPECT_TRUE(compressor_.compress(input, compressed));
    
    // 验证压缩率（重复数据应该有很高的压缩率）
    EXPECT_LT(compressed.size(), input.size());
    
    EXPECT_TRUE(compressor_.decompress(compressed, decompressed));
    EXPECT_EQ(input, decompressed);
}

// 测试6：随机数据压缩（低压缩率）
TEST_F(HuffmanCompressionTest, RandomDataCompression) {
    // 生成1KB的随机数据
    std::vector<uint8_t> input = generateRandomData(1024);
    
    std::vector<uint8_t> compressed;
    std::vector<uint8_t> decompressed;

    EXPECT_TRUE(compressor_.compress(input, compressed));
    
    // 随机数据可能压缩率较低，甚至可能增大（因为存储了树结构）
    // 但解压后必须一致
    
    EXPECT_TRUE(compressor_.decompress(compressed, decompressed));
    EXPECT_EQ(input, decompressed);
}

// 测试7：大文件压缩解压
TEST_F(HuffmanCompressionTest, LargeFileCompression) {
    // 生成1MB的文本数据
    std::string pattern = "This is a test pattern for Huffman compression. ";
    std::vector<uint8_t> input;
    
    for (size_t i = 0; i < (1024 * 1024) / pattern.size(); i++) {
        input.insert(input.end(), pattern.begin(), pattern.end());
    }
    
    std::vector<uint8_t> compressed;
    std::vector<uint8_t> decompressed;

    EXPECT_TRUE(compressor_.compress(input, compressed));
    
    // 大文件应该有明显压缩效果
    EXPECT_LT(compressed.size(), input.size());
    
    EXPECT_TRUE(compressor_.decompress(compressed, decompressed));
    EXPECT_EQ(input, decompressed);
}

// 测试8：二进制数据压缩解压
TEST_F(HuffmanCompressionTest, BinaryDataCompression) {
    std::vector<uint8_t> input;
    
    // 包含所有可能的字节值（0-255）
    for (int i = 0; i < 256; i++) {
        input.push_back(static_cast<uint8_t>(i));
    }
    // 重复几次以增加数据量
    for (int rep = 0; rep < 100; rep++) {
        for (int i = 0; i < 256; i++) {
            input.push_back(static_cast<uint8_t>(i));
        }
    }
    
    std::vector<uint8_t> compressed;
    std::vector<uint8_t> decompressed;

    EXPECT_TRUE(compressor_.compress(input, compressed));
    EXPECT_TRUE(compressor_.decompress(compressed, decompressed));
    EXPECT_EQ(input, decompressed);
}

// 测试9：压缩无效数据解压失败
TEST_F(HuffmanCompressionTest, InvalidDataDecompression) {
    std::vector<uint8_t> invalidData = {1, 2, 3, 4, 5};  // 无效的压缩格式
    std::vector<uint8_t> output;

    EXPECT_FALSE(compressor_.decompress(invalidData, output));
}

// 测试10：获取压缩器名称
TEST_F(HuffmanCompressionTest, GetName) {
    EXPECT_EQ(compressor_.getName(), "Huffman");
}

// 测试11：多次压缩解压一致性
TEST_F(HuffmanCompressionTest, MultipleCompressionDecompression) {
    std::string text = "Test multiple compression and decompression cycles.";
    std::vector<uint8_t> original(text.begin(), text.end());
    
    std::vector<uint8_t> current = original;
    
    // 进行3次压缩解压循环
    for (int cycle = 0; cycle < 3; cycle++) {
        std::vector<uint8_t> compressed;
        std::vector<uint8_t> decompressed;
        
        EXPECT_TRUE(compressor_.compress(current, compressed));
        EXPECT_TRUE(compressor_.decompress(compressed, decompressed));
        EXPECT_EQ(current, decompressed);
        
        current = decompressed;
    }
    
    // 最后应该和原始数据一致
    EXPECT_EQ(original, current);
}

// 测试12：压缩率测试（文本数据）
TEST_F(HuffmanCompressionTest, CompressionRatioForText) {
    // 典型的英文文本（字母频率分布不均匀）
    std::string text = "The quick brown fox jumps over the lazy dog. "
                       "This sentence contains all letters of the alphabet. "
                       "Huffman coding is efficient for text compression.";
    
    // 重复多次以增加数据量
    for (int i = 0; i < 100; i++) {
        text += "The quick brown fox jumps over the lazy dog. ";
    }
    
    std::vector<uint8_t> input(text.begin(), text.end());
    std::vector<uint8_t> compressed;
    
    EXPECT_TRUE(compressor_.compress(input, compressed));
    
    // 计算压缩率
    double compressionRatio = 100.0 * (1.0 - static_cast<double>(compressed.size()) / input.size());
    
    std::cout << "Original size: " << input.size() << " bytes" << std::endl;
    std::cout << "Compressed size: " << compressed.size() << " bytes" << std::endl;
    std::cout << "Compression ratio: " << compressionRatio << "%" << std::endl;
    
    // 文本数据应该有30-50%的压缩率
    EXPECT_GT(compressionRatio, 20.0);  // 至少20%压缩率
    
    std::vector<uint8_t> decompressed;
    EXPECT_TRUE(compressor_.decompress(compressed, decompressed));
    EXPECT_EQ(input, decompressed);
}

// 测试13：所有字节相同频率（均匀分布）
TEST_F(HuffmanCompressionTest, UniformByteFrequency) {
    // 所有字节出现频率相同（压缩效果最差）
    std::vector<uint8_t> input;
    for (int rep = 0; rep < 50; rep++) {
        for (int byte = 0; byte < 256; byte++) {
            input.push_back(static_cast<uint8_t>(byte));
        }
    }
    
    std::vector<uint8_t> compressed;
    std::vector<uint8_t> decompressed;
    
    EXPECT_TRUE(compressor_.compress(input, compressed));
    
    // 均匀分布的数据，压缩率可能很低
    // 但数据必须完整还原
    
    EXPECT_TRUE(compressor_.decompress(compressed, decompressed));
    EXPECT_EQ(input, decompressed);
}