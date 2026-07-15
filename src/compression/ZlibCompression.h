#ifndef ZLIBCOMPRESSION_H
#define ZLIBCOMPRESSION_H

#include "ICompressionStrategy.h"
#include <string>

/**
 * @brief Zlib压缩策略实现
 *
 * 使用zlib库实现数据的压缩和解压缩。
 * 支持多种压缩级别（0-9），默认使用默认压缩级别（6）。
 */
class ZlibCompression : public ICompressionStrategy {
public:
    /**
     * @brief 构造函数
     * @param compressionLevel 压缩级别（0-9）
     *        0 = 无压缩
     *        1 = 最快速度，最低压缩率
     *        9 = 最慢速度，最高压缩率
     *        -1 = 默认级别（通常为6）
     */
    explicit ZlibCompression(int compressionLevel = -1);

    /**
     * @brief 析构函数
     */
    ~ZlibCompression() override = default;

    /**
     * @brief 压缩数据
     * @param input 输入数据
     * @param output 输出数据（压缩后）
     * @return true 压缩成功
     * @return false 压缩失败
     */
    bool compress(const std::vector<uint8_t>& input,
                  std::vector<uint8_t>& output) override;

    /**
     * @brief 解压缩数据
     * @param input 输入数据（压缩后）
     * @param output 输出数据（原始数据）
     * @return true 解压缩成功
     * @return false 解压缩失败
     */
    bool decompress(const std::vector<uint8_t>& input,
                    std::vector<uint8_t>& output) override;

    /**
     * @brief 获取压缩算法名称
     * @return "Zlib"
     */
    std::string getName() const override;

    /**
     * @brief 设置压缩级别
     * @param level 压缩级别（0-9或-1）
     */
    void setCompressionLevel(int level);

    /**
     * @brief 获取压缩级别
     * @return 当前压缩级别
     */
    int getCompressionLevel() const;

private:
    int compressionLevel_;  ///< 压缩级别

    /**
     * @brief 验证压缩级别是否有效
     * @param level 压缩级别
     * @return true 有效
     * @return false 无效
     */
    bool isValidCompressionLevel(int level) const;
};

#endif // ZLIBCOMPRESSION_H