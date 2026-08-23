#include "AES_CTR.h"
#include <openssl/err.h>
#include <algorithm>

AES_CTR::AES_CTR(const std::vector<unsigned char>& key, const std::vector<unsigned char>& iv)
    : key_(key), iv_(iv), current_counter_(iv) {

    // 检查密钥长度是否有效
    if (key.size() != 16 && key.size() != 24 && key.size() != 32) {
        throw std::runtime_error("Invalid key size. Must be 16, 24 or 32 bytes.");
    }

    // 检查IV长度是否有效
    if (iv.size() != 16) {
        throw std::runtime_error("Invalid IV size. Must be 16 bytes.");
    }

    // 初始化OpenSSL加密上下文
    ctx_ = EVP_CIPHER_CTX_new();
    if (!ctx_) {
        throw std::runtime_error("Failed to create EVP_CIPHER_CTX");
    }
}

AES_CTR::~AES_CTR() {
    if (ctx_) {
        EVP_CIPHER_CTX_free(ctx_);
    }
}

std::vector<unsigned char> AES_CTR::Process(const std::vector<unsigned char>& input) {
    if (input.empty()) {
        return {};
    }

    std::vector<unsigned char> padding(padding_);
    padding.insert(padding.end(), input.begin(), input.end());

    int allsize = padding.size();
    // 根据密钥长度选择适当的密码
    const EVP_CIPHER* cipher = nullptr;
    switch (key_.size()) {
    case 16: cipher = EVP_aes_128_ctr(); break;
    case 24: cipher = EVP_aes_192_ctr(); break;
    case 32: cipher = EVP_aes_256_ctr(); break;
    default: throw std::runtime_error("Invalid key size");
    }

    // 初始化加密操作，使用当前计数器
    if (1 != EVP_EncryptInit_ex(ctx_, cipher, nullptr, key_.data(), current_counter_.data())) {
        throw std::runtime_error("EncryptInit failed");
    }

    // 输出缓冲区(可能比输入稍大)
    std::vector<unsigned char> output(allsize + EVP_MAX_BLOCK_LENGTH);
    int len = 0;

    // 执行加密/解密
    if (1 != EVP_EncryptUpdate(ctx_, output.data(), &len, padding.data(), allsize)) {
        throw std::runtime_error("EncryptUpdate failed");
    }

    int final_len = 0;
    if (1 != EVP_EncryptFinal_ex(ctx_, output.data() + len, &final_len)) {
        throw std::runtime_error("EncryptFinal failed");
    }

    // 调整输出大小为实际数据大小
    output.erase(output.begin(), output.begin() + padding_);
    output.resize((len- padding_) + final_len);

    int used = allsize % 16;
    // 计算加密的块数并更新计数器
    size_t blocks_used = (input.size() + 15 + padding_) / 16; // 向上取整
    if (used != 0)
        blocks_used--;
    padding_ = used;
    increment_counter(blocks_used);

    return output;
}

std::vector<unsigned char> AES_CTR::get_current_counter() const {
    return current_counter_;
}

void AES_CTR::increment_counter(size_t block) {
    for (int i = 0;i < block;i++) {
        // 以大端方式递增计数器
        for (int j = 15; j >= 0; --j) {
            if (j == 11)
                break;
            if (++current_counter_[j] != 0)
                break;
            // 如果字节溢出，继续处理前一个字节
        }
    }
}