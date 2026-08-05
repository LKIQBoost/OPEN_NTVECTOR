#pragma once
#include <string>
#include <json/json.h>
#include <random>
#include "Base64Cpp.h"
#include "FileUtils.h"
#include "SkinConverter.h"
#include "stb_image.h"
#include "stb_image_write.h"
#include "Logger.h"
class MinecraftClientDataUtils
{
public:
    static std::string GetClientData(std::string skin_path, std::string model_path, std::string player_name) {
        std::random_device rd;
        std::mt19937_64 gen(rd());
        std::uniform_int_distribution<uint64_t> dist;
        uint64_t r = dist(gen);
        std::string client_random_id = std::to_string(r);
        int width, height, channels;
        Json::FastWriter write;
        Json::Value root;
        if (stbi_info(skin_path.c_str(), &width, &height, &channels)) {
            root["UIProfile"] = 0;
            root["CapeOnClassicSkin"] = false;
            root["SkinResourcePatch"] = "ewogICAiZ2VvbWV0cnkiIDogewogICAgICAiZGVmYXVsdCIgOiAiZ2VvbWV0cnkuaHVtYW5vaWQuY3VzdG9tIgogICB9Cn0K";
            root["SkinGeometryData"] = Base64Cpp::base64_encode(FileUtils::FileConetnt(model_path));
            root["SkinImageWidth"] = width;
            root["CapeData"] = "";
            root["ThirdPartyNameOnly"] = false;
            root["DeviceId"] = player_name;
            root["IsReconnect"] = false;
            root["ClientRandomId"] = client_random_id;
            root["ServerAddress"] = "";
            root["PlatformOnlineId"] = "";
            root["CapeId"] = "-1";
            root["SkinAnimationData"] = "";
            root["GameVersion"] = "1.21.120";
            root["LanguageCode"] = "zh_CN";
            root["SkinIID"] = "-1";
            root["SkinColor"] = "#0";
            root["CurrentInputMode"] = 1;
            root["CompatibleWithClientSideChunkGen"] = true;
            root["SkinGeometryDataEngineVersion"] = "MC4wLjA=";
            root["DefaultInputMode"] = 1;
            root["SkinImageHeight"] = height;
            root["PremiumSkin"] = true;
            root["DeviceModel"] = "Win32";
            root["SelfSignedId"] = "";
            root["ThirdPartyName"] = player_name;
            root["BloomData"] = "";
            root["DeviceOS"] = 8;
            root["PlayFabId"] = "";
            root["SkinId"] = "";
            root["PersonaSkin"] = true;
            std::vector<uint8_t> data = SkinConverter::pngToSkinData(FileUtils::readFileToBinary(skin_path), width, height);
            root["SkinData"] = Base64Cpp::base64_encode(std::string(data.begin(), data.end()));
            root["PersonaPieces"] = Json::Value(Json::arrayValue);
            root["PieceTintColors"] = Json::Value(Json::arrayValue);
            root["IsEditorMode"] = false;
            root["TrustedSkin"] = false;
            root["GuiScale"] = 0;
            root["OverrideSkin"] = false;
            root["ArmSize"] = "wide";
            root["CapeImageHeight"] = 0;
            root["PlatformOfflineId"] = "";
            root["AnimatedImageData"] = Json::Value(Json::arrayValue);
            root["GrowthLevel"] = 1;
            root["CapeImageWidth"] = 0;
            return write.write(root);
        }
        else {
            Logger::getInstance().logv(LOG_ERROR, "[skin] Image parsing failed.");
            return std::string();
        }
    }
};

