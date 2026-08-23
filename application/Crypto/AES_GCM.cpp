#include "AES_GCM.h"

#include <openssl/rand.h>
#include <openssl/err.h>
#include <stdexcept>
#include <iostream>

AesGcm::AesGcm(const std::vector<unsigned char>& key, const std::vector<unsigned char>& iv, bool decrypt)
    : key_(key), iv_(iv) {
    // 初始化OpenSSL上下文
    ctx_ = EVP_CIPHER_CTX_new();
    if (!ctx_) {
        throw std::runtime_error("Failed to create EVP_CIPHER_CTX");
    }
    if (decrypt) {
        // 初始化密钥和IV
        if (1 != EVP_DecryptInit_ex(ctx_, EVP_aes_256_gcm(), NULL, key_.data(), iv_.data())) {
            std::cerr << "Failed to set key and IV" << std::endl;
            return;
        }
    }
    else {
        // 初始化密钥和IV
        if (1 != EVP_EncryptInit_ex(ctx_, EVP_aes_256_gcm(), NULL, key_.data(), iv_.data())) {
            std::cerr << "Failed to set key and IV" << std::endl;
            return;
        }
    }
}

AesGcm::~AesGcm() {
    // 清理OpenSSL上下文
    if (ctx_) {
        EVP_CIPHER_CTX_free(ctx_);
    }
}

bool AesGcm::encrypt(const std::vector<unsigned char>& plaintext,
    std::vector<unsigned char>& ciphertext) {
    int len;
    int ciphertext_len;

    // 分配足够的空间给密文
    ciphertext.resize(plaintext.size() + EVP_CIPHER_CTX_block_size(ctx_));

    // 提供要加密的明文
    if (1 != EVP_EncryptUpdate(ctx_, ciphertext.data(), &len, plaintext.data(), plaintext.size())) {
        std::cerr << "Failed to encrypt plaintext" << std::endl;
        return false;
    }
    //ciphertext_len = len;

    // 完成加密过程
    //if (1 != EVP_EncryptFinal_ex(ctx_, ciphertext.data() + len, &len)) {
    //    std::cerr << "Failed to finalize encryption" << std::endl;
    //    return false;
    //}
    //ciphertext_len;

    // 调整密文大小为实际大小
    ciphertext.resize(plaintext.size());


    return true;
}
bool AesGcm::encrypt(std::vector<unsigned char>& data) {
    if (data.empty()) {
        return true;
    }

    int len;

    // AES-GCM 是流密码模式，加密后大小不变
    // 但我们仍然需要临时缓冲区来避免覆盖问题
   // std::vector<unsigned char> ciphertext(data.size());

    // 执行加密到临时缓冲区
    if (1 != EVP_EncryptUpdate(ctx_, data.data(), &len, data.data(), data.size())) {
        std::cerr << "Failed to encrypt plaintext" << std::endl;
        return false;
    }

    // 将加密数据移回原数据
    //data = std::move(ciphertext);

    return true;
}

bool AesGcm::decrypt(const std::vector<unsigned char>& ciphertext) {
    int len;
    int plaintext_len;


    // 分配足够的空间给明文
    //plaintext.resize(ciphertext.size() + EVP_CIPHER_CTX_block_size(ctx_));

    // 提供要解密的密文
    if (1 != EVP_DecryptUpdate(ctx_, (unsigned char*)ciphertext.data(), &len, ciphertext.data(), ciphertext.size())) {
        std::cerr << "Failed to decrypt ciphertext" << std::endl;
        return false;
    }
    //plaintext_len = len;


    // 完成解密过程并验证认证标签
    //int ret = EVP_DecryptFinal_ex(ctx_, plaintext.data(), &len);
    //if (ret <= 0) {
    //    std::cerr << "Failed to verify authentication tag" << std::endl;
    //    return false;
    //}
    //plaintext_len;

    // 调整明文大小为实际大小
    //plaintext.resize(ciphertext.size());

    return true;
}

size_t AesGcm::getIvSize() {
    return 12; // GCM推荐使用12字节的IV
}

size_t AesGcm::getTagSize() {
    return 16; // 通常使用16字节的认证标签
}

std::vector<unsigned char> AesGcm::generateRandomIv() {
    std::vector<unsigned char> iv(getIvSize());
    if (1 != RAND_bytes(iv.data(), iv.size())) {
        throw std::runtime_error("Failed to generate random IV");
    }
    return iv;
}