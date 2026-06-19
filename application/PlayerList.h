#pragma once
#include "PacketBase.h"
#include <array>

// PlayerList 动作类型
constexpr uint8_t PlayerListActionAdd = 0;
constexpr uint8_t PlayerListActionRemove = 1;

// ===== 网易版 Skin 结构 (简化版) =====

// 玩家皮肤结构 (网易版协议)
struct PlayerSkin {
    std::string SkinID;                    // 皮肤唯一标识符
    std::string PlayFabID;                 // 平台皮肤ID
    std::string SkinResourcePatch;         // JSON 皮肤资源补丁
    uint32_t SkinImageWidth = 0;           // 皮肤图像宽度
    uint32_t SkinImageHeight = 0;          // 皮肤图像高度
    std::vector<uint8_t> SkinData;         // RGBA 皮肤像素数据
    uint32_t AnimationCount = 0;           // 动画数量 (网易版只读取数量)
    uint32_t CapeImageWidth = 0;           // 披风宽度
    uint32_t CapeImageHeight = 0;          // 披风高度
    std::string CapeData;                  // 披风图像数据 (网易版是空字符串)
    std::vector<uint8_t> SkinGeometry;     // 皮肤几何数据
    std::vector<uint8_t> GeometryDataEngineVersion;  // 几何引擎版本
    std::string AnimationData;             // 动画数据 (网易版是空字符串)
    std::string CapeID;                    // 披风标识符
    std::string FullID;                    // 完整皮肤ID
    std::string ArmSize;                   // 手臂尺寸 ("wide" 或 "slim")
    std::string SkinColour;                // 皮肤基础颜色 (如 "#b37b62")
    std::vector<std::string> PersonaPieces;      // Persona 部件列表 (字符串)
    std::vector<std::string> PieceTintColours;   // Persona 颜色列表 (字符串)
    bool PremiumSkin = false;              // 是否高级皮肤
    bool PersonaSkin = false;              // 是否 Persona 皮肤
    bool PersonaCapeOnClassicSkin = false; // 经典皮肤上的 Persona 披风
    bool PrimaryUser = false;              // 主用户
    bool OverrideAppearance = false;       // 是否覆盖外观
    bool Trusted = false;                  // 是否可信 (网易版在皮肤内读取)
};

// ===== PlayerListEntry 结构 (网易版) =====

struct PlayerListEntry {
    std::array<uint8_t, 16> UUID{};    // 玩家 UUID
    int64_t EntityUniqueID = 0;        // 实体唯一 ID (zigzag 编码)
    std::string Username;              // 玩家名称
    std::string XUID;                  // Xbox Live ID
    std::string PlatformChatID;        // 平台聊天 ID
    int32_t BuildPlatform = 0;         // 构建平台
    PlayerSkin Skin;                   // 玩家皮肤
    bool Teacher = false;              // 是否为教师 (教育版)
    bool Host = false;                 // 是否为房主
    bool SubClient = false;            // 是否为子客户端 (网易特有)
    int32_t Unknown = 0;               // 未知字段 (网易特有)
};

// ===== PlayerList 数据包 =====

class PlayerList : public PacketBase
{
public:
    unsigned char ID() override;
    void Deserializ(std::vector<unsigned char> pack) override;
    std::vector<unsigned char> Serializ() override;

    // 数据包字段
    uint8_t ActionType = 0;                    // 动作类型 (Add/Remove)
    std::vector<PlayerListEntry> Entries;      // 玩家列表条目

    // 便捷方法
    std::vector<std::string> GetPlayerNames() const;
    bool IsAddAction() const { return ActionType == PlayerListActionAdd; }
    bool IsRemoveAction() const { return ActionType == PlayerListActionRemove; }
};
