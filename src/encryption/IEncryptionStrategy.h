#ifndef IENCRYPTIONSTRATEGY_H
#define IENCRYPTIONSTRATEGY_H

#include <vector>
#include <cstdint>
#include <string>

/**
 * @brief 加密策略接口
 *
 * 定义加密和解密的抽象接口，支持多种加密算法的实现。
 * 采用策略模式，允许在运行时切换不同的加密算法。
 */
class IEncryptionStrategy {
public:
    /**
     * @brief 虚析构函数
     */
    virtual ~IEncryptionStrategy() = default;

    /**
     * @brief 加密数据
     *
     * @param plaintext 明文数据（原始数据）
     * @param ciphertext 密文数据（加密后的数据）
     * @param password 加密密码
     * @return true 加密成功
     * @return false 加密失败
     */
    virtual bool encrypt(const std::vector<uint8_t>& plaintext,
                         std::vector<uint8_t>& ciphertext,
                         const std::string& password) = 0;

    /**
     * @brief 解密数据
     *
     * @param ciphertext 密文数据（加密后的数据）
     * @param plaintext 明文数据（原始数据）
     * @param password 解密密码
     * @return true 解密成功
     * @return false 解密失败
     */
    virtual bool decrypt(const std::vector<uint8_t>& ciphertext,
                         std::vector<uint8_t>& plaintext,
                         const std::string& password) = 0;

    /**
     * @brief 获取加密算法名称
     *
     * @return 算法名称字符串
     */
    virtual std::string getName() const = 0;
};

#endif // IENCRYPTIONSTRATEGY_H