#ifndef AES_CTR_H
#define AES_CTR_H

#include <vector>
#include <stdexcept>
#include <openssl/evp.h>

class AES_CTR {
public:
    /**
     * @brief 构造函数，使用指定的密钥和初始向量初始化AES-CTR
     * @param key 加密密钥(16,24或32字节)
     * @param iv 初始向量(16字节)
     * @throws std::runtime_error 如果密钥或IV长度无效
     */
    AES_CTR(const std::vector<unsigned char>& key, const std::vector<unsigned char>& iv);

    /**
     * @brief 析构函数，清理OpenSSL资源
     */
    ~AES_CTR();

    // 禁止拷贝构造和拷贝赋值
    AES_CTR(const AES_CTR&) = delete;
    AES_CTR& operator=(const AES_CTR&) = delete;

    /**
     * @brief 加密或解密数据(CTR模式下两者相同)
     * @param input 输入数据
     * @return 处理后的数据
     * @throws std::runtime_error 如果加密/解密操作失败
     */
    std::vector<unsigned char> Process(const std::vector<unsigned char>& input);

    /**
     * @brief 获取当前计数器状态，可用于恢复加密进度
     * @return 当前计数器值
     */
    std::vector<unsigned char> get_current_counter() const;

private:
    void increment_counter(size_t block);

    std::vector<unsigned char> key_;     // 加密密钥
    std::vector<unsigned char> iv_;      // 初始向量
    std::vector<unsigned char> current_counter_; // 当前计数器状态
    EVP_CIPHER_CTX* ctx_ = nullptr;     // OpenSSL加密上下文
    int padding_ = 0;
};

#endif // AES_CTR_H