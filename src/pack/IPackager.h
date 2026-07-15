#ifndef IPACKAGER_H
#define IPACKAGER_H

#include <vector>
#include <cstdint>
#include <string>
#include "FileInfo.h"

/**
 * @brief 文件打包器接口
 *
 * 定义文件打包和解包的抽象接口，支持将多个文件打包为一个连续文件。
 * 采用策略模式，允许实现不同的打包格式。
 */
class IPackager {
public:
    /**
     * @brief 虚析构函数
     */
    virtual ~IPackager() = default;

    /**
     * @brief 打包多个文件为一个文件
     *
     * @param files 要打包的文件列表
     * @param output 输出打包数据（包含所有文件数据和元数据）
     * @return true 打包成功
     * @return false 打包失败
     */
    virtual bool pack(const std::vector<FileInfo>& files,
                      std::vector<uint8_t>& output) = 0;

    /**
     * @brief 解包打包文件为多个文件
     *
     * @param input 输入打包数据
     * @param files 输出文件列表（包含文件信息和数据）
     * @param outputDir 解包的目标目录
     * @return true 解包成功
     * @return false 解包失败
     */
    virtual bool unpack(const std::vector<uint8_t>& input,
                        std::vector<FileInfo>& files,
                        const std::string& outputDir) = 0;

    /**
     * @brief 获取打包格式名称
     *
     * @return 打包格式名称字符串
     */
    virtual std::string getName() const = 0;
};

#endif // IPACKAGER_H