#pragma once
#include <openssl/evp.h>
#include <string>

// 一次成功登录的产物 - 值对象
//
// 包含 MC 第一个 LoginPacket 需要的所有数据:
//   - chain:    服务器下发的证书链(已被本地 reissue)
//   - skinJwt:  签了名的皮肤数据 JWT
//   - ecKey:    用于后续 ECDH 握手的客户端 EC 密钥
//
// 拥有 EVP_PKEY* 的所有权,析构时释放
// 不可拷贝(EC 密钥不能两个所有者),可移动
class LoginSession {
public:
    LoginSession() = default;

    ~LoginSession() {
        if (m_ecKey) EVP_PKEY_free(m_ecKey);
    }

    LoginSession(const LoginSession&) = delete;
    LoginSession& operator=(const LoginSession&) = delete;

    LoginSession(LoginSession&& other) noexcept
        : chain(std::move(other.chain)),
        skinJwt(std::move(other.skinJwt)),
        m_ecKey(other.m_ecKey)
    {
        other.m_ecKey = nullptr;
    }

    LoginSession& operator=(LoginSession&& other) noexcept {
        if (this != &other) {
            if (m_ecKey) EVP_PKEY_free(m_ecKey);
            chain = std::move(other.chain);
            skinJwt = std::move(other.skinJwt);
            m_ecKey = other.m_ecKey;
            other.m_ecKey = nullptr;
        }
        return *this;
    }

    // === MC LoginPacket 用的字段 ===
    std::string chain;
    std::string skinJwt;

    // === ECDH 握手用 ===
    EVP_PKEY* getECKey() const { return m_ecKey; }

    // 工厂方法专用,转移 EVP_PKEY 所有权
    void setECKey(EVP_PKEY* key) {
        if (m_ecKey) EVP_PKEY_free(m_ecKey);
        m_ecKey = key;
    }

    bool valid() const {
        return !chain.empty() && m_ecKey != nullptr;
    }

private:
    EVP_PKEY* m_ecKey = nullptr;
};