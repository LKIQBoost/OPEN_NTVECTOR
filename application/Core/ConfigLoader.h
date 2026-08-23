#pragma once
#include "FileUtils.h"
#include <json/json.h>
#define CONFIGE "client_cfg.json"
#define SKINCONF "skin_data.json"
#define FILE_FOUND "File not found: "
#define FILE_CONFIG_LOAD "Loading Config from file: "
#define FILE_SKIN_DATA_LOAD "Loading SkinData from file: "
class ConfigLoader
{
public:
	static bool use_mcp;
	static std::string SkinData;
    static void LoadConfig(const std::string& client_data) {
		LOG(LOG_FILE, "[ConfigLoader] Starting configuration load");
		string filefound(FILE_FOUND);
		string clientcfg(CONFIGE);
		string akidtata(client_data);

		if (FileUtils::fileExists(CONFIGE)) {
			LOG(LOG_FILE, "[ConfigLoader] Loading client config from: ", clientcfg);
			Logger::getInstance().log_(FILE_CONFIG_LOAD + clientcfg);
			string data = FileUtils::FileConetnt(CONFIGE);
			Json::Reader reader = Json::Reader();
			Json::Value root = Json::Value();
			if (!reader.parse(data, root)) {
				LOG(LOG_ERROR, "[ConfigLoader] JSON parse failed for: ", clientcfg);
				Logger::getInstance().log(LOG_ERROR, "[JSON] The file does not exist or is malformed: " + clientcfg);
			}

			if (root.isMember("no_launch_mcp")) {
				if(!use_mcp)
					use_mcp = root["no_launch_mcp"].asBool();
				LOG(LOG_FILE, "[ConfigLoader] use_mcp set to: ", use_mcp);
			}
			else {
				LOG(LOG_ERROR, "[ConfigLoader] Missing 'no_launch_mcp' field in config");
				Logger::getInstance().log(LOG_ERROR, "[JSON] File format error: " + clientcfg);
			}
		}
		else {
			LOG(LOG_ERROR, "[ConfigLoader] Config file not found: ", clientcfg);
			Logger::getInstance().log(LOG_ERROR, filefound + clientcfg);
		}



		SkinData = string();
		if (FileUtils::fileExists(akidtata.data())) {
			LOG(LOG_FILE, "[ConfigLoader] Loading skin data from: ", akidtata);
			Logger::getInstance().log_(FILE_SKIN_DATA_LOAD + akidtata);
			SkinData = FileUtils::FileConetnt(akidtata);
			LOG(LOG_FILE, "[ConfigLoader] Skin data loaded, size: ", SkinData.size());
		}
		else {
			LOG(LOG_WARN, "[ConfigLoader] Skin data file not found: ", SKINCONF);
			Logger::getInstance().log(LOG_ERROR, filefound + SKINCONF);
		}
		LOG(LOG_FILE, "[ConfigLoader] Configuration load completed");
	}
};