#pragma once
#include <string>
#include <vector>
class Params {
public:
    static std::string HostNethernetId;
    static std::string FromNethernetId;
	static std::vector<std::string> params;
	static std::string ServerIP;
	static int ServerPort;
	static std::string MD5Token;
	static std::string DisplayName;
	static std::string UserID;
	static std::string EngineVersion;
	static std::string PatchVersion;
	static std::string GameVersion;
	static std::string AuthServerUrl;
	static std::string NeteaseServerID;
    static std::string Expand;
    static bool g79;
	static bool disout;
	static bool logger;
	static bool script_mcp;
    static bool AutoAuthInput;
    static bool RequestChunkRadius;

	static long long PlayerEntityID;
    static int launcher_port;
};