// application.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
// 注意:Python 的链接方式(Py_ENABLE_SHARED / Py_NO_ENABLE_SHARED)**不要**在这里
// 定义 —— 必须由 CMake 统一提供(见 application/CMakeLists.txt),否则本 TU
// 会和其余 TU 取到不同的 pyconfig.h 分支,轻则 dllimport 找不到符号,重则
// 数据结构布局不一致。
//#define Linker_NtUniSdk

//#define Linker_EnvSdk
//#define Linker_NtUniSdk
//#define ZLIB_WINAPI
#include <Python.h>
#include <iostream>
#include <stdlib.h>
#include <cassert>
#include "RakNetClient.h"
#include "StartupParams.h"
#include "FileUtils.h"
#include "Base64Cpp.h"
#include "ECDSA.h"
#include "ECC.h"
#include "AES_GCM.h"
#include "Logger.h"
#include "ConfigLoader.h"
#include "ConnectInstance.h"
#include "PacketBase.h"
#include "PacketCommon.h"
#include "MCPHash.h"
#include "SkinConverter.h"
#include "PythonUtils.h"
#include "CallbackManager.h"
#include "MCPFileSystem.h"
#include "PythonRuntime.h"
#include "StringSplitUtils.h"
#include "LoginAuth.h"
#include "WebSockeVirtualWrapper.h"
#include "rtc/rtc.hpp"
#include "ClientInstance.h"
#include "MPayWrapper.h"
#include "MinecraftClientDataUtils.h"
//#include "NewLogger.h"
/*
   Json::FastWriter write;
   Json::Value root;
   root["content"] = "Hello JsonCpp";
   std::cout << write.write(root);
   */
   //CreateClient("115.236.125.93", 17084);
   //std::cin.get();
   //Disconnection();



#ifdef Linker_NtUniSdk
#include "NtUniSdkBase.h"
#include "MPayDelegate.h"

NtUniSDK::INtUniSdkGamerInterface* ctx;
#endif // !Linker_NtUniSdk


#define VANILLA_MCP "vanilla.mcp"
unsigned int MinecraftBedrockProtocolVersion = 860;
ClientInstance* g_client_instance;
//ConnectInstance* RakNet_connect;
//assert(false, "error error error error");
bool use_mcp = true;

#ifdef _WIN32
bool enableVTMode() {
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hOut == INVALID_HANDLE_VALUE) return false;

	DWORD dwMode = 0;
	if (!GetConsoleMode(hOut, &dwMode)) return false;

	dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	return SetConsoleMode(hOut, dwMode);
}
#else
bool enableVTMode() { return true; }
#endif

// 日志回调函数
void LogHookCallback(const char* message) {
	if (Params::logger)
		std::cout << message << std::endl;
}


int main(int argc, char* argv[])
{
    //config__INIT
    bool _start_nethernet = false;
    bool _config_ctx_start_host_game = false;
    bool _nt_sdk_start = false;
    std::string event_name;
    std::string config_file = "mc.cfg";
    std::string skin_data_path = "";
    std::string skin_model_path = "";
    std::string XUID = "";
    std::string NUID = "";

    std::string client_data_path = "skin_data.json";
    std::string client_data = "";
    std::string operator_name = "";

	Params::g79 = false;
	Params::disout = false;
	Params::logger = false;
	Params::script_mcp = true;
    //end__config__INIT


	if (argc > 1 || FileUtils::fileExists(config_file)) {
		std::vector<std::string> parm;
		for (size_t i = 0; i < argc; i++)
		{
			parm.push_back(std::string(argv[i]));
		}
        Params::params = parm;
		for (size_t i = 0; i < argc; i++)
		{
			if (parm[i] == "--pause") {
				std::cin.get();
			}
			if (parm[i] == "--disout") {
				Params::disout = true;
			}
            if (parm[i] == "--logger") {
                Params::logger = true;
            }
            if (parm[i] == "--auto_auth_input") {
                Params::AutoAuthInput = true;
            }
            if (parm[i] == "--no_request_chunk") {
                Params::RequestChunkRadius = true;
            }
			if (parm[i] == "--script_mcp") {
				ConfigLoader::use_mcp = true;
			}
            if (parm[i] == "--start_from_launcher=1") {
                _nt_sdk_start = true;
            }
            if (parm[i] == "--start_nethernet") {
                _start_nethernet = true;
            }
            if (parm[i] == "--do_main") {
                if (!((i + 1) > (argc - 1))) {
                    _config_ctx_start_host_game = true;
                    event_name = parm[i + 1];
                }
            }
            if (parm[i] == "--HostNethernetId") {
                if (!((i + 1) > (argc - 1))) {
                    Params::HostNethernetId = parm[i + 1];
                }
            }
            if (parm[i] == "--FromNethernetId") {
                if (!((i + 1) > (argc - 1))) {
                    Params::FromNethernetId = parm[i + 1];
                }
            }
            if (parm[i] == "--xuid") {
                if (!((i + 1) > (argc - 1))) {
                    XUID = parm[i + 1];
                }
            }
            if (parm[i] == "--operator_uid") {
                if (!((i + 1) > (argc - 1))) {
                    NUID = parm[i + 1];
                }
            }
			if (parm[i] == "--DisplayName") {
				if (!((i + 1) > (argc - 1))) {
					Params::DisplayName = WindowsEncodingConverter::gbkToUtf8(parm[i + 1]);
				}
			}
			if (parm[i] == "--UserID") {
				if (!((i + 1) > (argc - 1))) {
					Params::UserID = parm[i + 1];
				}
			}
			if (parm[i] == "--EngineVersion") {
				if (!((i + 1) > (argc - 1))) {
					Params::EngineVersion = parm[i + 1];
				}
			}
			if (parm[i] == "--PatchVersion") {
				if (!((i + 1) > (argc - 1))) {
					Params::PatchVersion = parm[i + 1];
				}
			}
			if (parm[i] == "--NeteaseServerID") {
				if (!((i + 1) > (argc - 1))) {
					Params::NeteaseServerID = parm[i + 1];
				}
			}
			if (parm[i] == "--ServerIP") {
				if (!((i + 1) > (argc - 1))) {
					Params::ServerIP = parm[i + 1];
				}
			}
			if (parm[i] == "--ServerPort") {
				if (!((i + 1) > (argc - 1))) {
					Params::ServerPort = std::stoi(parm[i + 1]);
				}
			}
			if (parm[i] == "--ExpandParams") {
				if (!((i + 1) > (argc - 1))) {
					Params::Expand = parm[i + 1];
				}
			}
			if (parm[i] == "--AuthServerUrl") {
				if (!((i + 1) > (argc - 1))) {
					Params::AuthServerUrl = parm[i + 1];
				}
			}
			if (parm[i] == "--MD5Token") {
				if (!((i + 1) > (argc - 1))) {
					Params::MD5Token = Base64Cpp::base64_decode(parm[i + 1]);
				}
			}
            if (parm[i] == "--skin_image_path") {
                if (!((i + 1) > (argc - 1))) {
                    skin_data_path = parm[i + 1];
                }
            }
            if (parm[i] == "--skin_model_path") {
                if (!((i + 1) > (argc - 1))) {
                    skin_model_path = parm[i + 1];
                }
            }
            if (parm[i] == "--operator_name") {
                if (!((i + 1) > (argc - 1))) {
                    operator_name = parm[i + 1];
                }
            }
            if (parm[i] == "--operator_client_data") {
                if (!((i + 1) > (argc - 1))) {
                    client_data_path = parm[i + 1];
                }
            }
			if (parm[i] == "--g79") {
				Params::g79 = true;
			}
            if (parm[i] == "--config") {
                if (!((i + 1) > (argc - 1))) {
                    config_file = parm[i + 1];
                }
            }
            if (parm[i] == "--protocol") {
                if (!((i + 1) > (argc - 1))) {
                    MinecraftBedrockProtocolVersion = std::stoul(parm[i + 1]);
                }
            }
		}
		if (!enableVTMode()) {
			//std::cout << "警告: 无法启用ANSI支持，将无法打印日志！" << std::endl;
		}
		else {
			if (Params::logger) {
				Logger::getInstance().initialize();
				//rtc::InitLogger(rtc::LogLevel::Debug);
			}
		}
		Logger::getInstance().log_("[Device Lost] The graphics context was gained", LOG_WARN);
		//Logger::getInstance().log(LOG_INFO, "[Main] Application starting...");
		LOG(LOG_INFO, "[Main] Command line arguments: ", argc);
		LOG(LOG_INFO, "[Main] Logger enabled: ", Params::logger);
		LOG(LOG_INFO, "[Main] Script MCP: ", Params::script_mcp);
		LOG(LOG_INFO, "[Main] Platform mode (g79): ", Params::g79);

#ifdef Linker_EnvSdk

#endif
#ifdef Linker_NtUniSdk
#endif // DEBUG

        LOG(LOG_FILE, "[Main] Loading configuration...");
        ConfigLoader::LoadConfig(client_data_path);
        LOG(LOG_FILE, "[Main] Configuration loaded successfully");
        if (FileUtils::fileExists(config_file)) {
            std::string loader_config = FileUtils::FileConetnt(config_file);
            Json::CharReaderBuilder reader;
            Json::Value root;
            std::string errs;

            std::istringstream s(loader_config);
            if (!Json::parseFromStream(reader, s, &root, &errs)) {
                LOG(LOG_ERROR, "[config] Failed to parse startup configuration file.");
                return 1;
            }
            if (root.isMember("room_info")) {
                Params::ServerIP = root["room_info"]["ip"].asString();
                Params::ServerPort = root["room_info"]["port"].asInt();
            }
            else {
                LOG(LOG_ERROR, "[config] Failed to start configuration file parsing: no room_info");
            }
            if (root.isMember("player_info")) {
                Params::UserID = root["player_info"]["user_id"].asString();
                Params::DisplayName = root["player_info"]["user_name"].asString();
                operator_name = root["player_info"]["operator_name"].asString();
                Params::MD5Token = Base64Cpp::base64_decode(root["player_info"]["token"].asString());
            }
            else {
                LOG(LOG_ERROR, "[config] Failed to start configuration file parsing: no player_info");
            }
            if (root.isMember("misc")) {
                Params::AuthServerUrl = root["misc"]["auth_server_url"].asString();
                Params::launcher_port = root["misc"]["launcher_port"].asInt();
                Params::EngineVersion = root["misc"]["engine_version"].asString();
                Params::PatchVersion = root["misc"]["patch_version"].asString();
                Params::NeteaseServerID = root["misc"]["netease_sid"].asString();
                Params::g79 = root["misc"]["g79"].asBool();
                client_data = FileUtils::FileConetnt(root["misc"]["client_data_path"].asString());
            }
            else {
                LOG(LOG_ERROR, "[config] Failed to start configuration file parsing: no misc");
            }
            if (root.isMember("skin_info")) {
                skin_data_path = root["skin_info"]["skin"].asString();
                skin_model_path = root["skin_info"]["model"].asString();
            }
            else {
                LOG(LOG_ERROR, "[config] Failed to start configuration file parsing: no skin_info");
            }
        }
        if (client_data.empty()) {
            client_data = ConfigLoader::SkinData;
            if (!skin_data_path.empty() || !skin_model_path.empty()) {
                if (operator_name.empty()) {
                    operator_name = Params::DisplayName;
                }
                client_data = MinecraftClientDataUtils::GetClientData(skin_data_path, skin_model_path, operator_name);
            }
        }
        LOG(LOG_SCRIPTING, "[Main] Initialize ClientInstance.");
        g_client_instance = new ClientInstance();

		LOG(LOG_SCRIPTING, "[Main] Starting Python runtime...");
	    PythonRuntime::startUp();
		LOG(LOG_SCRIPTING, "[Main] Python runtime initialized");
		

		//Logger::getInstance().log(LOG_WARN, "该分发版本未接入python，可能遇到未知错误。");
        if (_start_nethernet) {
            // ★ 新:LAN/NetherNet 路径
            LOG(LOG_INFO, "[Main] NetherNet mode - direct startup");

            // 检查必填参数
            if (Params::DisplayName.empty() || Params::UserID.empty() ||
                Params::MD5Token.empty()) {
                LOG(LOG_ERROR, "[Main] NetherNet mode missing required params");
                if (Params::disout) exit(0);
                return 1;
            }

            std::string host_nid = Params::HostNethernetId;
            std::string from_nid = Params::FromNethernetId;
            std::string sig_ip = Params::ServerIP;     // 复用 --ServerIP
            int         sig_port = Params::ServerPort;   // 复用 --ServerPort
            uint32_t    uid = (uint32_t)std::stoul(Params::UserID);
            std::string md5_token_b64 = Base64Cpp::base64_encode(Params::MD5Token);

            SSL_library_init();
            OpenSSL_add_all_algorithms();

            // 本地登录
            LoginSession session = LoginAuth::LoginLocal(
                Params::DisplayName, Params::UserID, client_data, XUID, NUID);
            if (!session.valid()) {
                LOG(LOG_ERROR, "[Main] LoginLocal failed");
                if (Params::disout) exit(0);
                return 1;
            }

            g_client_instance->startUp_NetherNet(
                std::move(session),
                host_nid, from_nid, md5_token_b64,
                sig_ip, sig_port, uid);

            while (true) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }
        }
        else if (!_config_ctx_start_host_game && !_nt_sdk_start) {
            // 原 RakNet 公网服路径
        }
        else {
            // 原 Python 脚本路径
        }

		if (!_config_ctx_start_host_game && !_nt_sdk_start) {
			LOG(LOG_INFO, "[Main] Preparing authentication chain pair...");
			ChainPair Pair;
			Pair.AuthServerUrl = Params::AuthServerUrl;
			Pair.DisplayName = Params::DisplayName;
			Pair.EngineVersion = Params::EngineVersion;
			Pair.PatchVersion = Params::PatchVersion;
			Pair.MD5Token = Params::MD5Token;
			Pair.UserID = Params::UserID;
			Pair.NeteaseServerID = Params::NeteaseServerID;
			LOG(LOG_INFO, "[Main] ChainPair configured:");
			LOG(LOG_INFO, "[Main] - DisplayName: ", Pair.DisplayName);
			LOG(LOG_INFO, "[Main] - UserID: ", Pair.UserID);
			LOG(LOG_INFO, "[Main] - EngineVersion: ", Pair.EngineVersion);
			LOG(LOG_INFO, "[Main] - PatchVersion: ", Pair.PatchVersion);
			LOG(LOG_INFO, "[Main] - ServerID: ", Pair.NeteaseServerID);
			LOG(LOG_INFO, "[Main] - AuthServer: ", Pair.AuthServerUrl);
			if (Pair.NeteaseServerID.empty()) {
				LOG(LOG_ERROR, "[Main] Validation failed: serverid is empty");
				cout << "the serverid is empty";
				exit(0);
			}
			if (Pair.AuthServerUrl.empty()) {
				LOG(LOG_ERROR, "[Main] Validation failed: authserver is empty");
				cout << "the authserver is empty";
				exit(0);
			}
			if (Pair.DisplayName.empty()) {
				LOG(LOG_ERROR, "[Main] Validation failed: name is empty");
				cout << "the name is empty";
				exit(0);
			}
			if (Pair.EngineVersion.empty()) {
				LOG(LOG_ERROR, "[Main] Validation failed: engine version is empty");
				cout << "the engine version is empty";
				exit(0);
			}
			if (Pair.MD5Token.empty()) {
				LOG(LOG_ERROR, "[Main] Validation failed: token is empty");
				cout << "the token is empty";
				exit(0);
			}
			if (Pair.UserID.empty()) {
				LOG(LOG_ERROR, "[Main] Validation failed: uid is empty");
				cout << "the uid is empty";
				exit(0);
			}
			LOG(LOG_INFO, "[Main] ChainPair validation passed");
			LOG(LOG_INFO, "[Main] Initializing OpenSSL...");
			SSL_library_init();
			OpenSSL_add_all_algorithms();
			LOG(LOG_INFO, "[Main] OpenSSL initialized");
			LOG(LOG_INFO, "[Main] Authenticating with server...");

            PythonEventEngine e;
            e.trigger("on_start_ready");
            LoginSession session = LoginAuth::Login(Pair, client_data);
            if (session.valid()) {
                g_client_instance->startUp(std::move(session), Params::ServerIP, Params::ServerPort);
                //LOG(LOG_INFO, "[Main] Connection initiated, entering main loop");
            }
            else {
                LOG(LOG_ERROR, "[Main] Authentication failed");
                if (Params::disout)
                    exit(0);
            }
			//return 0;
			while (true) {
				std::this_thread::sleep_for(std::chrono::milliseconds(1000));
				//string data;
				//cin >> data;
			}
		}
		else {
            if (_nt_sdk_start) {
#ifdef Linker_NtUniSdk
                PythonEventEngine e;
                e.trigger("on_init_uni_sdk");
                Logger::getInstance().log_("InitUniSdk!!!!!!!!");
                MPayWrapper mw;
                mw.doLogin();
#else
#endif // DEBUG
            }
            if (_config_ctx_start_host_game) {
                PythonEventEngine engine;
                engine.trigger(event_name);
            }
			//std::unique_ptr<WebSocketClient> wsc = std::make_unique<WebSocketClient>("45.253.177.72", 8899, "Minecraft-Bedrock", "/4034328500471339769/2881323365/1MYbM/W6znswDilR/8NBNA==/lqb546Yzvm5HL93jAL3BZw==");
			//wsc->connect();
			//分支逻辑，直接触发事件，供python调用，参数同host game事件
			PythonEventEngine engine;
			engine.trigger("on_event_name");
			while (true) {
				std::this_thread::sleep_for(std::chrono::milliseconds(1000));
				//string data;
				//cin >> data;
			}
		}
	}
	else {
        /*
        PlayerAuthInput a;
        a.headRotation.x = 54.75494385f;
        a.headRotation.y = -94.53570557f;
        a.position.x = 54.47731781f;
        a.position.y = 99.62001038f;
        a.position.z = 18.59784508f;
        a.MoveVector.x = 0;
        a.MoveVector.y = 0;
        a.headYaw = -94.53570557f;
        //a.inputData.push_back((PlayerAuthInputData)0x18);
        //a.inputData.push_back((PlayerAuthInputData)0x31);
        a.inputData.push_back((PlayerAuthInputData)0x33);
        std::vector<uint8_t> s = a.Serializ();
        cout << Easy::StringToHex(std::string((char*)s.data(), s.size())) << '\n';
        */
        Params::logger = true;
        enableVTMode();
        Logger::getInstance().initialize();
        Logger::getInstance().log_("[Device Lost] The graphics context was gained", LOG_WARN);
        //LOG(LOG_WARN, "[Main] No arguments provided, running in standalone mode");
	}
}