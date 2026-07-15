#pragma once

#include "ICompressionStrategy.h"
#include <map>
#include <queue>
#include <string>
#include <vector>
#include <cstdint>

namespace DataBackup {

// Huffman树节点结构
struct HuffmanNode {
    uint8_t byte;           // 字节值（叶子节点有效）
    uint64_t frequency;     // 频率
    HuffmanNode* left;      // 左子节点
    HuffmanNode* right;     // 右子节点
    bool isLeaf;            // 是否为叶子节点

    // 构造函数（叶子节点）
    HuffmanNode(uint8_t b, uint64_t freq)
        : byte(b), frequency(freq), left(nullptr), right(nullptr), isLeaf(true) {}

    // 构造函数（内部节点）
    HuffmanNode(HuffmanNode* l, HuffmanNode* r)
        : byte(0), frequency(l->frequency + r->frequency), 
          left(l), right(r), isLeaf(false) {}

    // 用于优先队列的比较函数（频率小的优先）
    bool operator>(const HuffmanNode& other) const {
        return frequency > other.frequency;
    }
};

// 比较器（用于优先队列）
struct HuffmanNodeComparator {
    bool operator()(HuffmanNode* a, HuffmanNode* b) const {
        return a->frequency > b->frequency;  // 频率小的优先
    }
};

/**
 * @brief Huffman压缩实现类
 * 
 * 使用哈夫曼编码进行数据压缩，基于字节频率构建最优二叉树，
 * 为高频字节分配短编码，低频字节分配长编码。
 */
class HuffmanCompression : public ICompressionStrategy {
public:
    /**
     * @brief 构造函数
     */
    HuffmanCompression();

    /**
     * @brief 析构函数
     */
    ~HuffmanCompression();

    /**
     * @brief 压缩数据
     * @param input 输入数据（原始数据）
     * @param output 输出数据（压缩后的数据）
     * @return true 压缩成功
     * @return false 压缩失败
     */
    bool compress(const std::vector<uint8_t>& input,
                  std::vector<uint8_t>& output) override;

    /**
     * @brief 解压缩数据
     * @param input 输入数据（压缩后的数据）
     * @param output 输出数据（原始数据）
     * @return true 解压缩成功
     * @return false 解压缩失败
     */
    bool decompress(const std::vector<uint8_t>& input,
                    std::vector<uint8_t>& output) override;

    /**
     * @brief 获取压缩算法名称
     * @return 算法名称 "Huffman"
     */
    std::string getName() const override { return "Huffman"; }

private:
    HuffmanNode* root_;     // Huffman树根节点（用于解压时临时存储）

    /**
     * @brief 构建字节频率表
     * @param data 输入数据
     * @return 字节频率映射表
     */
    std::map<uint8_t, uint64_t> buildFrequencyTable(const std::vector<uint8_t>& data);

    /**
     * @brief 构建Huffman树
     * @param freqTable 字节频率表
     * @return Huffman树根节点
     */
    HuffmanNode* buildHuffmanTree(const std::map<uint8_t, uint64_t>& freqTable);

    /**
     * @brief 生成编码表（递归遍历树）
     * @param node 当前节点
     * @param currentCode 当前编码路径（左0右1）
     * @param codeTable 编码表（输出）
     */
    void generateCodeTable(HuffmanNode* node, 
                           const std::string& currentCode,
                           std::map<uint8_t, std::string>& codeTable);

    /**
     * @brief 序列化Huffman树（用于存储到压缩数据中）
     * @param node 当前节点
     * @param output 输出数据流
     */
    void serializeTree(HuffmanNode* node, std::vector<uint8_t>& output);

    /**
     * @brief 反序列化Huffman树（从压缩数据中恢复树）
     * @param input 输入数据流
     * @param index 当前读取位置（会被更新）
     * @return Huffman树节点
     */
    HuffmanNode* deserializeTree(const std::vector<uint8_t>& input, size_t& index);

    /**
     * @brief 编码数据（将字节序列转换为比特流）
     * @param data 输入数据
     * @param codeTable 编码表
     * @param output 输出比特流（字节形式存储）
     */
    void encodeData(const std::vector<uint8_t>& data,
                    const std::map<uint8_t, std::string>& codeTable,
                    std::vector<uint8_t>& output);

    /**
     * @brief 解码数据（将比特流转换为字节序列）
     * @param input 输入比特流
     * @param root Huffman树根节点
     * @param bitCount 有效比特数量
     * @param startIndex 比特流起始索引
     * @param output 输出字节序列
     */
    void decodeData(const std::vector<uint8_t>& input,
                    HuffmanNode* root,
                    size_t bitCount,
                    size_t startIndex,
                    std::vector<uint8_t>& output);

    /**
     * @brief 删除Huffman树（释放内存）
     * @param node 当前节点
     */
    void deleteTree(HuffmanNode* node);
};

} // namespace DataBackup