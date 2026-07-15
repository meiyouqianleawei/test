#include <gtest/gtest.h>
#include "encryption/XOREncryption.h"
#include <vector>
#include <string>

using namespace DataBackup;

class XOREncryptionTest : public ::testing::Test {
protected:
    XOREncryption encryption;

    void SetUp() override {
        encryption = XOREncryption();
    }
};

// 测试1：空数据加密
TEST_F(XOREncryptionTest, EncryptEmptyData) {
    std::vector<uint8_t> plaintext;
    std::vector<uint8_t> ciphertext;
    std::string password = "test123";

    EXPECT_TRUE(encryption.encrypt(plaintext, ciphertext, password));
    EXPECT_TRUE(ciphertext.empty());
}

// 测试2：空密码加密（应该失败）
TEST_F(XOREncryptionTest, EncryptWithEmptyPassword) {
    std::vector<uint8_t> plaintext = {1, 2, 3, 4, 5};
    std::vector<uint8_t> ciphertext;
    std::string password = "";

    EXPECT_FALSE(encryption.encrypt(plaintext, ciphertext, password));
}

// 测试3：简单文本加密解密
TEST_F(XOREncryptionTest, SimpleTextEncryptionDecryption) {
    std::string text = "Hello World!";
    std::vector<uint8_t> plaintext(text.begin(), text.end());
    std::vector<uint8_t> ciphertext;
    std::vector<uint8_t> decrypted;
    std::string password = "secret";

    EXPECT_TRUE(encryption.encrypt(plaintext, ciphertext, password));
    EXPECT_NE(plaintext, ciphertext);  // 加密后应该不同

    EXPECT_TRUE(encryption.decrypt(ciphertext, decrypted, password));
    EXPECT_EQ(plaintext, decrypted);  // 解密后应该还原
}

// 测试4：二进制数据加密解密
TEST_F(XOREncryptionTest, BinaryDataEncryptionDecryption) {
    std::vector<uint8_t> plaintext = {0x00, 0xFF, 0x55, 0xAA, 0x12, 0x34};
    std::vector<uint8_t> ciphertext;
    std::vector<uint8_t> decrypted;
    std::string password = "binary_test";

    EXPECT_TRUE(encryption.encrypt(plaintext, ciphertext, password));
    EXPECT_TRUE(encryption.decrypt(ciphertext, decrypted, password));
    EXPECT_EQ(plaintext, decrypted);
}

// 测试5：大文件加密解密（>1MB）
TEST_F(XOREncryptionTest, LargeFileEncryptionDecryption) {
    size_t fileSize = 1024 * 1024 + 100;  // 1MB + 100字节
    std::vector<uint8_t> plaintext(fileSize);

    // 生成随机数据
    for (size_t i = 0; i < fileSize; ++i) {
        plaintext[i] = static_cast<uint8_t>(i % 256);
    }

    std::vector<uint8_t> ciphertext;
    std::vector<uint8_t> decrypted;
    std::string password = "large_file_test";

    EXPECT_TRUE(encryption.encrypt(plaintext, ciphertext, password));
    EXPECT_EQ(plaintext.size(), ciphertext.size());

    EXPECT_TRUE(encryption.decrypt(ciphertext, decrypted, password));
    EXPECT_EQ(plaintext, decrypted);
}

// 测试6：不同密码加密（应该产生不同密文）
TEST_F(XOREncryptionTest, DifferentPasswords) {
    std::string text = "Same Data";
    std::vector<uint8_t> plaintext(text.begin(), text.end());

    std::vector<uint8_t> ciphertext1;
    std::vector<uint8_t> ciphertext2;

    EXPECT_TRUE(encryption.encrypt(plaintext, ciphertext1, "password1"));
    EXPECT_TRUE(encryption.encrypt(plaintext, ciphertext2, "password2"));

    EXPECT_NE(ciphertext1, ciphertext2);  // 不同密码应该产生不同密文
}

// 测试7：错误密码解密（应该产生错误数据）
TEST_F(XOREncryptionTest, WrongPasswordDecryption) {
    std::string text = "Secret Data";
    std::vector<uint8_t> plaintext(text.begin(), text.end());
    std::vector<uint8_t> ciphertext;
    std::vector<uint8_t> decrypted;

    EXPECT_TRUE(encryption.encrypt(plaintext, ciphertext, "correct_password"));
    EXPECT_TRUE(encryption.decrypt(ciphertext, decrypted, "wrong_password"));

    EXPECT_NE(plaintext, decrypted);  // 错误密码解密应该产生错误数据
}

// 测试8：重复数据加密（检查密钥流循环）
TEST_F(XOREncryptionTest, RepeatingDataEncryption) {
    std::vector<uint8_t> plaintext(1000);
    for (size_t i = 0; i < plaintext.size(); ++i) {
        plaintext[i] = 'A';  // 重复数据
    }

    std::vector<uint8_t> ciphertext;
    std::vector<uint8_t> decrypted;
    std::string password = "test";

    EXPECT_TRUE(encryption.encrypt(plaintext, ciphertext, password));
    EXPECT_TRUE(encryption.decrypt(ciphertext, decrypted, password));
    EXPECT_EQ(plaintext, decrypted);
}

// 测试9：获取算法名称
TEST_F(XOREncryptionTest, GetName) {
    EXPECT_EQ(encryption.getName(), "XOR");
}

// 测试10：多次加密解密循环
TEST_F(XOREncryptionTest, MultipleEncryptionDecryption) {
    std::string text = "Test Data";
    std::vector<uint8_t> plaintext(text.begin(), text.end());
    std::string password = "test";

    for (int round = 0; round < 5; ++round) {
        std::vector<uint8_t> ciphertext;
        std::vector<uint8_t> decrypted;

        EXPECT_TRUE(encryption.encrypt(plaintext, ciphertext, password));
        EXPECT_TRUE(encryption.decrypt(ciphertext, decrypted, password));
        EXPECT_EQ(plaintext, decrypted);
    }
}

// 测试11：长密码加密解密
TEST_F(XOREncryptionTest, LongPasswordEncryption) {
    std::string text = "Short Data";
    std::vector<uint8_t> plaintext(text.begin(), text.end());
    std::string password = "This_is_a_very_long_password_for_testing";

    std::vector<uint8_t> ciphertext;
    std::vector<uint8_t> decrypted;

    EXPECT_TRUE(encryption.encrypt(plaintext, ciphertext, password));
    EXPECT_TRUE(encryption.decrypt(ciphertext, decrypted, password));
    EXPECT_EQ(plaintext, decrypted);
}

// 测试12：中文字符加密解密
TEST_F(XOREncryptionTest, ChineseCharactersEncryption) {
    std::string text = "中文测试数据";
    std::vector<uint8_t> plaintext(text.begin(), text.end());
    std::vector<uint8_t> ciphertext;
    std::vector<uint8_t> decrypted;
    std::string password = "中文密码";

    EXPECT_TRUE(encryption.encrypt(plaintext, ciphertext, password));
    EXPECT_TRUE(encryption.decrypt(ciphertext, decrypted, password));
    EXPECT_EQ(plaintext, decrypted);
}