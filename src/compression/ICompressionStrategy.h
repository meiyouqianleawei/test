#ifndef ICOMPRESSIONSTRATEGY_H
#define ICOMPRESSIONSTRATEGY_H

#include <vector>
#include <string>
#include <cstdint>

/**
 * @brief 压缩策略接口
 *
 * 定义压缩和解压缩的抽象接口，支持多种压缩算法的实现。
 * 采用策略模式，允许在运行时切换不同的压缩算法。
 */
class ICompressionStrategy {
public:
    /**
     * @brief 虚析构函数
     */
    virtual ~ICompressionStrategy() = default;

    /**
     * @brief 压缩数据
     *
     * @param input 输入数据（原始数据）
     * @param output 输出数据（压缩后的数据）
     * @return true 压缩成功
     * @return false 压缩失败
     */
    virtual bool compress(const std::vector<uint8_t>& input,
                          std::vector<uint8_t>& output) = 0;

    /**
     * @brief 解压缩数据
     *
     * @param input 输入数据（压缩后的数据）
     * @param output 输出数据（原始数据）
     * @return true 解压缩成功
     * @return false 解压缩失败
     */
    virtual bool decompress(const std::vector<uint8_t>& input,
                             std::vector<uint8_t>& output) = 0;

    /**
     * @brief 获取压缩算法名称
     *
     * @return 算法名称字符串
     */
    virtual std::string getName() const = 0;
};

#endif // ICOMPRESSIONSTRATEGY_H