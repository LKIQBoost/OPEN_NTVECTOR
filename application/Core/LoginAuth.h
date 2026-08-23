#pragma once
#include "json/json.h"
#include <string>
#include "httplib.h"
#include "EasyUtils.cpp"
#include "ECC.h"
#include "Base64Cpp.h"
#include "ECDSA.h"
#include "SkinParams.h"
#include "StartupParams.h"
#include "WindowsEncodingConverter.h"
#include "LoginSession.h"
#include "engine_wrapper.h"
#pragma warning(disable: 4996)

#define MOJANG_PUBLIC_KEY "MHYwEAYHKoZIzj0CAQYFK4EEACIDYgAEEsmU+IF/XeAF3yiqJ7Ko36btx6JtdB26wV9Eyw4AYR/nmesznkfXxwQ4B0NkSnGIZccbb2f3nFUYughKSoAcNHx+lQm8F9h9RwhrNgeN907z06LUA2AqWcwqasxyaU0E"
#define AUTHENTICATION_V2URL "/authentication-v2"

using namespace std;
using namespace Json;

class ClientInstance;

struct ChainPair
{
    std::string MD5Token;
    std::string DisplayName;
    std::string UserID;
    std::string EngineVersion;
    std::string PatchVersion;
    std::string AuthServerUrl;
    std::string NeteaseServerID;
};

class LoginAuth {
public:
    // 公网服:HTTP 请求 mojang/网易认证服,本地 reissue 链
    static LoginSession Login(const ChainPair& pair, const std::string& rawSkinData);

    // P2P/LAN(NetherNet):本地自签,不调 HTTP
    static LoginSession LoginLocal(const std::string& displayName,
        const std::string& userId,
        const std::string& rawSkinData, std::string uuid = "", std::string player_uid = "");

    static std::string utf8ToUnicodeEscape(const std::string& utf8_str);

private:
    static EVP_PKEY* GenerateECPair(std::string* outPublicKeyBase64);
    static std::string FetchChainFromServer(const ChainPair& pair,
        const std::string& publicKeyBase64,
        EVP_PKEY* ecKey);
    static std::string ReissueChain(const std::string& rawChain,
        EVP_PKEY* ecKey,
        const std::string& publicKeyBase64);
    static std::string SignSkinData(const std::string& rawSkinData,
        EVP_PKEY* ecKey,
        const std::string& publicKeyBase64);

    // ★ 新增:LAN 模式自签证书链
    static std::string BuildLocalChain(const std::string& displayName,
        const std::string& userId,
        EVP_PKEY* ecKey,
        const std::string& publicKeyBase64, std::string& uuid, std::string& player_uid);

    // ★ 新增:基于 userId 生成稳定的 UUID
    static std::string GenerateIdentityUuid(const std::string& userId);
};