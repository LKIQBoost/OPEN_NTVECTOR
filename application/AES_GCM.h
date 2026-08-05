#pragma once
#include <openssl/evp.h>
#include <vector>
#include <string>

class AesGcm {
public:
    AesGcm() {}
    // ���캯����������Կ��IV����ʼ��������
    AesGcm(const std::vector<unsigned char>& key, const std::vector<unsigned char>& iv, bool decrypt = false);

    // ���ܺ���
    bool encrypt(const std::vector<unsigned char>& plaintext,
        std::vector<unsigned char>& ciphertext);
    bool encrypt(std::vector<unsigned char>& data);

    // ���ܺ���
    bool decrypt(const std::vector<unsigned char>& ciphertext);

    // ��ȡIV��С����̬������
    static size_t getIvSize();

    // ��ȡ��ǩ��С����̬������
    static size_t getTagSize();

    // �������IV����̬������
    static std::vector<unsigned char> generateRandomIv();

    // ��������
    ~AesGcm();

private:
    std::vector<unsigned char> key_;
    std::vector<unsigned char> iv_;
    EVP_CIPHER_CTX* ctx_;
};
