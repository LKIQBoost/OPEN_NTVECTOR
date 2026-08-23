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
            // 自动提取模型中的 geometry 标识符，支持自定义 4D 模型皮肤：
            //   新格式  {"minecraft:geometry":[{"description":{"identifier":...}}]}
            //   旧格式  {"geometry.xxx":{...}}  (网易/经典 4D 皮肤)
            // 找不到时兜底为默认 geometry.humanoid.custom
            std::string model_content = FileUtils::FileConetnt(model_path);
            std::string default_geometry = "geometry.humanoid.custom";
            Json::Value mroot;
            Json::Reader mreader = Json::Reader();
            if (mreader.parse(model_content, mroot)) {
                if (mroot.isMember("minecraft:geometry") && mroot["minecraft:geometry"].isArray()
                    && mroot["minecraft:geometry"].size() > 0) {
                    // 新格式 {"minecraft:geometry":[...]}。
                    // 注意原版 model.json 含 [geometry.cape, geometry.humanoid.custom, customSlim]，
                    // 不能盲取 [0](那是披风)，必须优先选玩家几何体。
                    // 优先级: humanoid.custom > 任意含 humanoid 的 > 数组第一个(单几何体4D皮肤)
                    bool found = false;
                    std::string selected;
                    for (Json::Value::ArrayIndex i = 0; i < mroot["minecraft:geometry"].size() && !found; i++) {
                        Json::Value& geom = mroot["minecraft:geometry"][i];
                        if (geom.isMember("description") && geom["description"].isMember("identifier")
                            && geom["description"]["identifier"].asString() == "geometry.humanoid.custom") {
                            selected = "geometry.humanoid.custom";
                            found = true;
                        }
                    }
                    for (Json::Value::ArrayIndex i = 0; i < mroot["minecraft:geometry"].size() && !found; i++) {
                        Json::Value& geom = mroot["minecraft:geometry"][i];
                        if (geom.isMember("description") && geom["description"].isMember("identifier")) {
                            std::string id = geom["description"]["identifier"].asString();
                            if (id.find("humanoid") != std::string::npos) {
                                selected = id;
                                found = true;
                            }
                        }
                    }
                    for (Json::Value::ArrayIndex i = 0; i < mroot["minecraft:geometry"].size() && !found; i++) {
                        Json::Value& geom = mroot["minecraft:geometry"][i];
                        if (geom.isMember("description") && geom["description"].isMember("identifier")) {
                            selected = geom["description"]["identifier"].asString();
                            found = true;
                        }
                    }
                    if (found) default_geometry = selected;
                }
                else {
                    Json::Value::Members keys = mroot.getMemberNames();
                    for (size_t i = 0; i < keys.size(); i++) {
                        if (keys[i].compare(0, 9, "geometry.") == 0) {
                            default_geometry = keys[i];
                            break;
                        }
                    }
                }
            }
            Json::Value patch;
            patch["geometry"]["default"] = default_geometry;
            root["SkinResourcePatch"] = Base64Cpp::base64_encode(write.write(patch));
            root["SkinGeometryData"] = Base64Cpp::base64_encode(model_content);
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

