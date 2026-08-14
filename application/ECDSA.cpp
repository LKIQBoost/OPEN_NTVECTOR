#include "ECDSA.h"
std::string ECDSA::sign_es384(const std::string& data, EC_KEY* ec_key) {
    EVP_PKEY* pkey = EVP_PKEY_new();
    if (!pkey) {
        throw std::runtime_error("EVP_PKEY_new failed");
    }

    // 将 EC_KEY 封装进 EVP_PKEY
    if (EVP_PKEY_assign_EC_KEY(pkey, ec_key) != 1) {
        EVP_PKEY_free(pkey);
        throw std::runtime_error("EVP_PKEY_assign_EC_KEY failed");
    }

    EVP_MD_CTX* md_ctx = EVP_MD_CTX_new();
    if (!md_ctx) {
        EVP_PKEY_free(pkey);
        throw std::runtime_error("EVP_MD_CTX_new failed");
    }

    // 初始化签名上下文
    if (EVP_DigestSignInit(md_ctx, nullptr, EVP_sha384(), nullptr, pkey) != 1) {
        EVP_MD_CTX_free(md_ctx);
        EVP_PKEY_free(pkey);
        throw std::runtime_error("EVP_DigestSignInit failed");
    }

    // 更新数据
    if (EVP_DigestSignUpdate(md_ctx, data.data(), data.size()) != 1) {
        EVP_MD_CTX_free(md_ctx);
        EVP_PKEY_free(pkey);
        throw std::runtime_error("EVP_DigestSignUpdate failed");
    }

    // 获取签名长度
    size_t sig_len;
    if (EVP_DigestSignFinal(md_ctx, nullptr, &sig_len) != 1) {
        EVP_MD_CTX_free(md_ctx);
        EVP_PKEY_free(pkey);
        throw std::runtime_error("EVP_DigestSignFinal failed (1)");
    }

    // 执行签名
    std::vector<unsigned char> sig(sig_len);
    if (EVP_DigestSignFinal(md_ctx, sig.data(), &sig_len) != 1) {
        EVP_MD_CTX_free(md_ctx);
        EVP_PKEY_free(pkey);
        throw std::runtime_error("EVP_DigestSignFinal failed (2)");
    }

    EVP_MD_CTX_free(md_ctx);
    EVP_PKEY_free(pkey);  // 注意：这里会释放 EC_KEY，如果不需要提前释放，可以改用 EVP_PKEY_up_ref

    // 转换 DER 格式（ECDSA 签名是 R|S 结构）
    ECDSA_SIG* ec_sig = ECDSA_SIG_new();
    const unsigned char* p = sig.data();
    if (d2i_ECDSA_SIG(&ec_sig, &p, sig_len) == nullptr) {
        throw std::runtime_error("d2i_ECDSA_SIG failed");
    }

    const BIGNUM* r, * s;
    ECDSA_SIG_get0(ec_sig, &r, &s);

    std::vector<unsigned char> r_bn(48);  // P-384 曲线，R 和 S 各 48 字节
    std::vector<unsigned char> s_bn(48);
    BN_bn2binpad(r, r_bn.data(), 48);
    BN_bn2binpad(s, s_bn.data(), 48);

    ECDSA_SIG_free(ec_sig);

    // 合并 R 和 S
    std::vector<unsigned char> raw_sig;
    raw_sig.insert(raw_sig.end(), r_bn.begin(), r_bn.end());
    raw_sig.insert(raw_sig.end(), s_bn.begin(), s_bn.end());

    return std::string(raw_sig.begin(), raw_sig.end());
}
void ECDSA::EVP_PKEY_AS_EC_KEY(EVP_PKEY* EVPKey, EC_KEY* ECKey) {
    if (EVP_PKEY_id(EVPKey) == EVP_PKEY_EC) {
        // 获取 EC_KEY (不增加引用计数)
        ECKey = EVP_PKEY_get1_EC_KEY(EVPKey);
    }
}
void ECDSA::EC_KEY_AS_EVP_PKEY(EVP_PKEY* EVPKey, EC_KEY* ECKey) {
    EVP_PKEY_assign_EC_KEY(EVPKey, ECKey);
}