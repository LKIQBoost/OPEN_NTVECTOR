#include "LoginAuth.h"
#include "Logger.h"
#include <ctime>
#include <iomanip>
#include <openssl/md5.h>

// ============================================================
// 公开入口
// ============================================================

LoginSession LoginAuth::Login(const ChainPair& pair, const std::string& rawSkinData) {
    LOG(LOG_INFO, "[LoginAuth] Login - Starting authentication flow");
    LOG(LOG_INFO, "[LoginAuth] Login - User: ", pair.DisplayName, ", UID: ", pair.UserID);

    LoginSession session;

    // 1. 生成 EC 密钥对
    std::string publicKeyBase64;
    EVP_PKEY* ecKey = GenerateECPair(&publicKeyBase64);
    if (!ecKey) {
        LOG(LOG_ERROR, "[LoginAuth] Login - GenerateECPair failed");
        return session;
    }
    session.setECKey(ecKey);
    LOG(LOG_INFO, "[LoginAuth] Login - EC pair generated, pubkey len=", publicKeyBase64.length());

    // 2. HTTP 请求拿原始 chain
    std::string rawChain = FetchChainFromServer(pair, publicKeyBase64, ecKey);
    if (rawChain.empty()) {
        LOG(LOG_ERROR, "[LoginAuth] Login - FetchChainFromServer failed");
        return LoginSession{};   // 析构会清掉 ecKey
    }

    // 3. 用本地 EC 密钥重新签 chain
    session.chain = ReissueChain(rawChain, ecKey, publicKeyBase64);
    if (session.chain.empty()) {
        LOG(LOG_ERROR, "[LoginAuth] Login - ReissueChain failed");
        return LoginSession{};
    }

    // 4. 签 skin data
    if (!rawSkinData.empty()) {
        session.skinJwt = SignSkinData(rawSkinData, ecKey, publicKeyBase64);
        if (session.skinJwt.empty()) {
            LOG(LOG_WARN, "[LoginAuth] Login - SignSkinData returned empty");
        }
    }

    LOG(LOG_INFO, "[LoginAuth] Login - Authentication completed successfully");
    return session;
}

// ============================================================
// 私有步骤实现
// ============================================================

EVP_PKEY* LoginAuth::GenerateECPair(std::string* outPublicKeyBase64) {
    std::string publicKey;
    std::string privateKey;

    EVP_PKEY* ecKey = ECC::GetECPair(&publicKey, &privateKey);
    if (!ecKey) {
        return nullptr;
    }

    // 移除换行,截取出公钥 base64 部分
    std::string clean;
    for (char c : publicKey) {
        if (c != '\n') clean.push_back(c);
    }
    if (clean.length() < 186) {
        LOG(LOG_ERROR, "[LoginAuth] GenerateECPair - Public key too short: ", clean.length());
        EVP_PKEY_free(ecKey);
        return nullptr;
    }
    clean = clean.substr(0, 186);
    clean = clean.substr(26, 160);

    *outPublicKeyBase64 = std::move(clean);
    return ecKey;
}

std::string LoginAuth::FetchChainFromServer(const ChainPair& pair,
    const std::string& publicKeyBase64,
    EVP_PKEY* ecKey)
{
    LOG(LOG_NETWORK, "[LoginAuth] FetchChainFromServer - target: ", pair.AuthServerUrl);

    // 构造请求体 JSON
    FastWriter write;
    Value root;
    if (!Params::g79) {
        LOG(LOG_INFO, "[LoginAuth] FetchChainFromServer - Platform: Windows/PC (32-bit)");
        root["bit"] = "32";
        root["clientKey"] = publicKeyBase64;
        root["displayName"] = pair.DisplayName;
        root["engineVersion"] = pair.EngineVersion;
        root["netease_sid"] = pair.NeteaseServerID;
        root["os_name"] = "windows";
        root["patchVersion"] = "";
        root["pcCheck"] = "0";
        root["platform"] = "pc";
        root["uid"] = (unsigned int)std::stoul(pair.UserID);
    }
    else {
        LOG(LOG_INFO, "[LoginAuth] FetchChainFromServer - Platform: Android (64-bit)");
        root["bit"] = "64";
        root["clientKey"] = publicKeyBase64;
        root["displayName"] = pair.DisplayName;
        root["engineVersion"] = pair.EngineVersion;
        root["netease_sid"] = pair.NeteaseServerID;
        root["os_name"] = "android";
        root["patchVersion"] = pair.PatchVersion;
        root["uid"] = (unsigned int)std::stoul(pair.UserID);
    }

    std::string body = write.write(root);
    body = body.substr(0, body.length() - 1);   // 去掉 FastWriter 尾部的 \n
    if (Params::logger) std::cout << body << std::endl;

    // 加密 body
    std::string encrypted;
    Easy::HttpEncrypt(&encrypted, &body);
    std::string hexBody = Easy::StringToHex(encrypted);

    // 计算动态 token
    std::string token;
    Easy::ComputeDynamicToken(pair.MD5Token, body, AUTHENTICATION_V2URL, &token);
    token = Easy::StringToHex(token, true);

    // 发请求
    httplib::Client cli(pair.AuthServerUrl);
    cli.enable_server_certificate_verification(false);
    httplib::Headers headers = {
        {"User-Agent", "libhttpclient/1.0.0.0"},
        {"user-token", token},
        {"user-id",    pair.UserID}
    };

    LOG(LOG_NETWORK, "[LoginAuth] FetchChainFromServer - POST ", AUTHENTICATION_V2URL);
    auto res = cli.Post(AUTHENTICATION_V2URL, headers, hexBody, "application/json");

    if (!res) {
        LOG(LOG_ERROR, "[LoginAuth] FetchChainFromServer - HTTP request failed (no response)");
        PythonEventEngine e;
        e.trigger("on_authentication_result", 0, std::string());
        return std::string();
    }

    if (res->status != 200) {
        LOG(LOG_ERROR, "[LoginAuth] FetchChainFromServer - HTTP status: ", res->status);
        PythonEventEngine e;
        e.trigger("on_authentication_result", res->status, std::string());
        return std::string();
    }

    LOG(LOG_INFO, "[LoginAuth] FetchChainFromServer - HTTP 200, decrypting response");
    std::string hexResp = res->body;
    std::string respBin = Easy::HexToString(hexResp);
    std::string decrypted;
    Easy::HttpDecrypt(&decrypted, &respBin);

    PythonEventEngine e;
    e.trigger("on_authentication_result", res->status, decrypted);

    return decrypted;
}

std::string LoginAuth::ReissueChain(const std::string& rawChain,
    EVP_PKEY* ecKey,
    const std::string& publicKeyBase64)
{
    LOG(LOG_INFO, "[LoginAuth] ReissueChain - Processing chain");

    Json::Value root;
    Json::CharReaderBuilder builder;
    JSONCPP_STRING errs;
    std::istringstream stream(rawChain);

    if (!Json::parseFromStream(builder, stream, &root, &errs)) {
        LOG(LOG_ERROR, "[LoginAuth] ReissueChain - JSON parse failed: ", errs);
        return std::string();
    }

    const Json::Value courses = root["chain"];
    if (!courses.isArray() || courses.size() < 2) {
        LOG(LOG_ERROR, "[LoginAuth] ReissueChain - Chain array invalid");
        return std::string();
    }

    // 解析第一段 JWT 拿到 nbf/exp
    std::string chain0 = courses[0].asString();
    std::string header;
    std::string payload;
    std::string signature;
    Easy::split_jwt(chain0, header, payload, signature);

    payload = Easy::base64url_decode_s(payload);
    payload = Base64Cpp::base64_decode(payload);
    header = Easy::base64url_decode_s(header);
    header = Base64Cpp::base64_decode(header);

    stream.clear();
    stream.str(payload);
    if (!Json::parseFromStream(builder, stream, &root, &errs)) {
        LOG(LOG_ERROR, "[LoginAuth] ReissueChain - Payload parse failed: ", errs);
        return std::string();
    }

    int nbf = root["nbf"].asInt();
    int exp = root["exp"].asInt();

    // 构造新的 payload(指向 mojang 公钥)
    Json::Value payloadRoot;
    payloadRoot["certificateAuthority"] = true;
    payloadRoot["exp"] = exp;
    payloadRoot["identityPublicKey"] = MOJANG_PUBLIC_KEY;
    payloadRoot["nbf"] = nbf;

    FastWriter write;
    std::string payloadStr = write.write(payloadRoot);

    // JWT header: 用我们的 publicKey 作 x5u
    payloadRoot.clear();
    payloadRoot["alg"] = "ES384";
    payloadRoot["x5u"] = publicKeyBase64;
    std::string headerStr = write.write(payloadRoot);

    // 签名
    std::string encodedHeader = Easy::base64url_encode_s(Base64Cpp::base64_encode(headerStr));
    std::string encodedPayload = Easy::base64url_encode_s(Base64Cpp::base64_encode(payloadStr));
    std::string signingInput = encodedHeader + "." + encodedPayload;

    EC_KEY* rawEcKey = EVP_PKEY_get1_EC_KEY(ecKey);
    std::string sigBytes = ECDSA::sign_es384(signingInput, rawEcKey);
    std::string newSignature = Easy::base64url_encode_s(Base64Cpp::base64_encode(sigBytes));
    std::string newJwt = signingInput + "." + newSignature;

    // 重组 chain
    payloadRoot.clear();
    Json::Value chainArr(Json::arrayValue);
    chainArr.append(newJwt);
    for (int i = 0; i < 2; i++) {
        chainArr.append(courses[i].asString());
    }
    payloadRoot["chain"] = chainArr;

    std::string newChain = write.write(payloadRoot);
    payloadRoot.clear();
    payloadRoot["Certificate"] = newChain;
    payloadRoot["AuthenticationType"] = 0;
    payloadRoot["Token"] = "";

    LOG(LOG_INFO, "[LoginAuth] ReissueChain - Chain reissued successfully");
    return write.write(payloadRoot);
}

std::string LoginAuth::SignSkinData(const std::string& rawSkinData,
    EVP_PKEY* ecKey,
    const std::string& publicKeyBase64)
{
    LOG(LOG_INFO, "[LoginAuth] SignSkinData - Input size: ", rawSkinData.size());

    FastWriter write;
    Json::Value headerJson;
    headerJson["alg"] = "ES384";
    headerJson["x5u"] = publicKeyBase64;
    std::string headerStr = write.write(headerJson);

    std::string encodedHeader = Easy::base64url_encode_s(Base64Cpp::base64_encode(headerStr));
    std::string encodedPayload = Easy::base64url_encode_s(Base64Cpp::base64_encode(rawSkinData));
    std::string signingInput = encodedHeader + "." + encodedPayload;

    EC_KEY* rawEcKey = EVP_PKEY_get1_EC_KEY(ecKey);
    std::string sigBytes = ECDSA::sign_es384(signingInput, rawEcKey);
    std::string signature = Easy::base64url_encode_s(Base64Cpp::base64_encode(sigBytes));

    std::string jwt = signingInput + "." + signature;
    LOG(LOG_INFO, "[LoginAuth] SignSkinData - JWT length: ", jwt.length());
    return jwt;
}

// ============================================================
// 工具方法(从原文件保留)
// ============================================================

std::string LoginAuth::utf8ToUnicodeEscape(const std::string& utf8_str) {
    std::ostringstream oss;
    for (size_t i = 0; i < utf8_str.length();) {
        unsigned char c = utf8_str[i];
        if (c < 128) {
            if (c >= 32 && c <= 126) {
                oss << c;
            }
            else {
                oss << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(c);
            }
            i++;
        }
        else {
            uint32_t code_point = 0;
            int following_bytes = 0;
            if ((c & 0xE0) == 0xC0) {
                code_point = c & 0x1F;
                following_bytes = 1;
            }
            else if ((c & 0xF0) == 0xE0) {
                code_point = c & 0x0F;
                following_bytes = 2;
            }
            else if ((c & 0xF8) == 0xF0) {
                code_point = c & 0x07;
                following_bytes = 3;
            }
            for (int j = 0; j < following_bytes && (i + 1 + j) < utf8_str.length(); j++) {
                unsigned char next_c = utf8_str[i + 1 + j];
                if ((next_c & 0xC0) == 0x80) {
                    code_point = (code_point << 6) | (next_c & 0x3F);
                }
            }
            oss << "\\u" << std::hex << std::setw(4) << std::setfill('0') << code_point;
            i += 1 + following_bytes;
        }
    }
    return oss.str();
}
LoginSession LoginAuth::LoginLocal(const std::string& displayName,
    const std::string& userId,
    const std::string& rawSkinData, std::string uuid, std::string player_uid)
{
    LOG(LOG_INFO, "[LoginAuth] LoginLocal - Starting local authentication (P2P mode)");
    LOG(LOG_INFO, "[LoginAuth] LoginLocal - User: ", displayName, ", UID: ", userId);

    LoginSession session;

    // 1. 生成 EC 密钥对
    std::string publicKeyBase64;
    EVP_PKEY* ecKey = GenerateECPair(&publicKeyBase64);
    if (!ecKey) {
        LOG(LOG_ERROR, "[LoginAuth] LoginLocal - GenerateECPair failed");
        return session;
    }
    session.setECKey(ecKey);

    // 2. 自签 chain
    session.chain = BuildLocalChain(displayName, userId, ecKey, publicKeyBase64, uuid, player_uid);
    if (session.chain.empty()) {
        LOG(LOG_ERROR, "[LoginAuth] LoginLocal - BuildLocalChain failed");
        return LoginSession{};
    }

    // 3. 签 skin (跟公网模式完全一样,复用 SignSkinData)
    if (!rawSkinData.empty()) {
        session.skinJwt = SignSkinData(rawSkinData, ecKey, publicKeyBase64);
    }

    LOG(LOG_INFO, "[LoginAuth] LoginLocal - Completed successfully");
    return session;
}

std::string LoginAuth::BuildLocalChain(const std::string& displayName,
    const std::string& userId,
    EVP_PKEY* ecKey,
    const std::string& publicKeyBase64, std::string& uuid, std::string& player_uid)
{
    long now = (long)time(nullptr);
    long exp = now + 365L * 24 * 3600;   // 1 年有效期

    // 构造 payload
    Json::Value extraData;
    extraData["XUID"] = "";
    extraData["displayName"] = displayName;
    if (uuid.empty()) {
        extraData["identity"] = GenerateIdentityUuid(userId);
    }
    else {
        extraData["identity"] = uuid;
    }
    extraData["netease_sid"] = "";
    if (player_uid.empty()) {
        extraData["netease_uid"] = userId;
    }
    else {
        extraData["netease_uid"] = player_uid;
    }

    Json::Value payload;
    payload["exp"] = (Json::Int64)exp;
    payload["extraData"] = extraData;
    payload["identityPublicKey"] = publicKeyBase64;
    payload["nbf"] = (Json::Int64)now;

    // 构造 header
    Json::Value header;
    header["alg"] = "ES384";
    header["x5u"] = publicKeyBase64;

    FastWriter write;
    std::string headerStr = write.write(header);
    std::string payloadStr = write.write(payload);

    // 签名
    std::string encodedHeader = Easy::base64url_encode_s(Base64Cpp::base64_encode(headerStr));
    std::string encodedPayload = Easy::base64url_encode_s(Base64Cpp::base64_encode(payloadStr));
    std::string signingInput = encodedHeader + "." + encodedPayload;

    EC_KEY* rawEcKey = EVP_PKEY_get1_EC_KEY(ecKey);
    std::string sigBytes = ECDSA::sign_es384(signingInput, rawEcKey);
    std::string signature = Easy::base64url_encode_s(Base64Cpp::base64_encode(sigBytes));
    std::string jwt = signingInput + "." + signature;

    // 包装成 chain 数组
    Json::Value chainArr(Json::arrayValue);
    chainArr.append(jwt);
    Json::Value chainRoot;
    chainRoot["chain"] = chainArr;
    std::string chainJson = write.write(chainRoot);

    // 包装成 LoginPacket Certificate
    Json::Value root;
    root["Certificate"] = chainJson;
    root["AuthenticationType"] = 2;     // ★ 关键:P2P/LAN 模式
    root["Token"] = "";
    return write.write(root);
}

std::string LoginAuth::GenerateIdentityUuid(const std::string& userId)
{
    // 简单方案:md5(userId),格式化成 UUID 形式
    // 这样同一个 userId 总是生成同一个 UUID(确定性)
    unsigned char hash[MD5_DIGEST_LENGTH];
    MD5(reinterpret_cast<const unsigned char*>(userId.data()), userId.size(), hash);

    // 设置版本位 = 3 (name-based UUID, MD5)
    hash[6] = (hash[6] & 0x0F) | 0x30;
    // 设置 variant 位
    hash[8] = (hash[8] & 0x3F) | 0x80;

    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (int i = 0; i < 16; i++) {
        oss << std::setw(2) << (int)hash[i];
        if (i == 3 || i == 5 || i == 7 || i == 9) oss << '-';
    }
    return oss.str();
}