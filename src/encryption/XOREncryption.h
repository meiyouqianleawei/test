#pragma once
#include "IEncryptionStrategy.h"
#include <vector>
#include <string>

namespace DataBackup {

/**
 * XOR加密策略
 * 使用密码生成密钥流，通过异或操作进行加密/解密
 */
class XOREncryption : public IEncryptionStrategy {
public:
    XOREncryption();

    bool encrypt(const std::vector<uint8_t>& plaintext,
                 std::vector<uint8_t>& ciphertext,
                 const std::string& password) override;

    bool decrypt(const std::vector<uint8_t>& ciphertext,
                 std::vector<uint8_t>& plaintext,
                 const std::string& password) override;

    std::string getName() const override { return "XOR"; }

private:
    // 生成密钥流（密码循环扩展）
    std::vector<uint8_t> generateKeyStream(const std::string& password, size_t dataLength);
};

} // namespace DataBackup