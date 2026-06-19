#pragma once
#include <openssl/evp.h>
#include <vector>
#include <string>

class AesGcm {
public:
    AesGcm() {}
    // 构造函数，接收密钥和IV（初始化向量）
    AesGcm(const std::vector<unsigned char>& key, const std::vector<unsigned char>& iv, bool decrypt = false);

    // 加密函数
    bool encrypt(const std::vector<unsigned char>& plaintext,
        std::vector<unsigned char>& ciphertext);
    bool encrypt(std::vector<unsigned char>& data);

    // 解密函数
    bool decrypt(const std::vector<unsigned char>& ciphertext);

    // 获取IV大小（静态方法）
    static size_t getIvSize();

    // 获取标签大小（静态方法）
    static size_t getTagSize();

    // 生成随机IV（静态方法）
    static std::vector<unsigned char> generateRandomIv();

    // 析构函数
    ~AesGcm();

private:
    std::vector<unsigned char> key_;
    std::vector<unsigned char> iv_;
    EVP_CIPHER_CTX* ctx_;
};
