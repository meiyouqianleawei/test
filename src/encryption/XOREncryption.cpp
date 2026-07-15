#include "XOREncryption.h"
#include <stdexcept>

namespace DataBackup {

XOREncryption::XOREncryption() {}

bool XOREncryption::encrypt(const std::vector<uint8_t>& plaintext,
                             std::vector<uint8_t>& ciphertext,
                             const std::string& password) {
    // 检查密码有效性
    if (password.empty()) {
        return false;
    }

    // 检查数据有效性
    if (plaintext.empty()) {
        ciphertext.clear();
        return true;  // 空数据加密成功，返回空密文
    }

    // 生成密钥流
    std::vector<uint8_t> keyStream = generateKeyStream(password, plaintext.size());

    // XOR加密
    ciphertext.resize(plaintext.size());
    for (size_t i = 0; i < plaintext.size(); ++i) {
        ciphertext[i] = plaintext[i] ^ keyStream[i];
    }

    return true;
}

bool XOREncryption::decrypt(const std::vector<uint8_t>& ciphertext,
                             std::vector<uint8_t>& plaintext,
                             const std::string& password) {
    // 检查密码有效性
    if (password.empty()) {
        return false;
    }

    // 检查数据有效性
    if (ciphertext.empty()) {
        plaintext.clear();
        return true;  // 空数据解密成功，返回空明文
    }

    // 生成密钥流（与加密时相同）
    std::vector<uint8_t> keyStream = generateKeyStream(password, ciphertext.size());

    // XOR解密（异或操作是可逆的）
    plaintext.resize(ciphertext.size());
    for (size_t i = 0; i < ciphertext.size(); ++i) {
        plaintext[i] = ciphertext[i] ^ keyStream[i];
    }

    return true;
}

std::vector<uint8_t> XOREncryption::generateKeyStream(const std::string& password, size_t dataLength) {
    std::vector<uint8_t> keyStream(dataLength);

    // 将密码循环扩展到数据长度
    size_t passwordLength = password.length();
    for (size_t i = 0; i < dataLength; ++i) {
        keyStream[i] = static_cast<uint8_t>(password[i % passwordLength]);
    }

    return keyStream;
}

} // namespace DataBackup