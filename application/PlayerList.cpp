#include "PlayerList.h"
#include <cstring>

// ===== 辅助函数：读取字符串列表 =====
static void ReadStringList(BinaryReader& br, std::vector<std::string>& list) {
    uint32_t count = br.ReadVarUInt();
    list.clear();
    list.reserve(count);
    for (uint32_t i = 0; i < count; i++) {
        std::string str;
        br.ReadStringUTF(str);
        list.push_back(std::move(str));
    }
}

// ===== 辅助函数：读取皮肤 (网易版) =====
static void ReadSkin(BinaryReader& br, PlayerSkin& skin) {
    // 基本信息
    br.ReadStringUTF(skin.SkinID);
    br.ReadStringUTF(skin.PlayFabID);
    br.ReadStringUTF(skin.SkinResourcePatch);  // 网易版是 string

    // 皮肤图像 (xi32 = 小端 int32)
    skin.SkinImageWidth = br.ReadUInt32();
    skin.SkinImageHeight = br.ReadUInt32();

    // 皮肤数据 (varint 长度前缀 + 字节数据)
    br.ReadByteSlice(skin.SkinData);

    // 动画数量 (xu32 = 小端 uint32, 网易版只读取数量不读取详细动画)
    skin.AnimationCount = br.ReadUInt32();

    // 披风图像 (xu32)
    skin.CapeImageWidth = br.ReadUInt32();
    skin.CapeImageHeight = br.ReadUInt32();

    // 披风数据 (网易版是空字符串)
    br.ReadStringUTF(skin.CapeData);

    // 几何数据 (varint 长度前缀 + 字节数据)
    br.ReadByteSlice(skin.SkinGeometry);
    br.ReadByteSlice(skin.GeometryDataEngineVersion);

    // 动画数据 (网易版是空字符串)
    br.ReadStringUTF(skin.AnimationData);

    // 字符串字段
    br.ReadStringUTF(skin.CapeID);
    br.ReadStringUTF(skin.FullID);
    br.ReadStringUTF(skin.ArmSize);
    br.ReadStringUTF(skin.SkinColour);

    // Persona 部件和颜色 (网易版是字符串列表)
    ReadStringList(br, skin.PersonaPieces);
    ReadStringList(br, skin.PieceTintColours);

    // 布尔标志
    br.ReadBool(skin.PremiumSkin);
    br.ReadBool(skin.PersonaSkin);
    br.ReadBool(skin.PersonaCapeOnClassicSkin);
    br.ReadBool(skin.PrimaryUser);
    br.ReadBool(skin.OverrideAppearance);
    br.ReadBool(skin.Trusted);  // 网易版在皮肤内读取
}

// ===== 辅助函数：读取 PlayerListEntry (Add 模式, 网易版) =====
static void ReadPlayerListEntry(BinaryReader& br, PlayerListEntry& entry) {
    // UUID (16 bytes)
    void* uuidData = br.Read(16);
    if (uuidData) {
        memcpy(entry.UUID.data(), uuidData, 16);
    }

    // EntityUniqueID (zigzag64)
    entry.EntityUniqueID = br.ReadVarInt64();

    // 字符串字段
    br.ReadStringUTF(entry.Username);
    br.ReadStringUTF(entry.XUID);
    br.ReadStringUTF(entry.PlatformChatID);

    // BuildPlatform (xi32, 小端)
    entry.BuildPlatform = br.ReadInt32();

    // Skin
    ReadSkin(br, entry.Skin);

    // 布尔标志
    br.ReadBool(entry.Teacher);
    br.ReadBool(entry.Host);
    br.ReadBool(entry.SubClient);  // 网易特有

    // 未知字段 (xi32)
    entry.Unknown = br.ReadInt32();  // 网易特有
}

// ===== 辅助函数：读取 PlayerListEntry (Remove 模式) =====
static void ReadPlayerListRemoveEntry(BinaryReader& br, PlayerListEntry& entry) {
    // 仅读取 UUID
    void* uuidData = br.Read(16);
    if (uuidData) {
        memcpy(entry.UUID.data(), uuidData, 16);
    }
}

// ===== PlayerList 实现 =====

unsigned char PlayerList::ID()
{
    return IDPlayerList;
}

void PlayerList::Deserializ(std::vector<unsigned char> pack)
{
    BinaryReader br(pack.data(), pack.size());

    // 1. 读取 ActionType
    ActionType = br.ReadUInt8();

    // 2. 读取条目数量 (varuint32)
    uint32_t entryCount = br.ReadVarUInt();
    Entries.clear();
    Entries.reserve(entryCount);

    // 3. 根据 ActionType 读取条目
    if (ActionType == PlayerListActionAdd) {
        // Add 模式：读取完整的玩家信息
        for (uint32_t i = 0; i < entryCount; i++) {
            PlayerListEntry entry;
            ReadPlayerListEntry(br, entry);
            Entries.push_back(std::move(entry));
        }
    }
    else if (ActionType == PlayerListActionRemove) {
        // Remove 模式：仅读取 UUID
        // 注意：Python 代码中使用 list_num + 1，但这可能是个 bug
        // 这里按照标准逻辑使用 entryCount
        for (uint32_t i = 0; i < entryCount; i++) {
            PlayerListEntry entry;
            ReadPlayerListRemoveEntry(br, entry);
            Entries.push_back(std::move(entry));
        }
    }
}

std::vector<unsigned char> PlayerList::Serializ()
{
    // PlayerList 通常是服务端发送的，客户端一般不需要序列化
    BinaryWriter bw(1024);

    bw.WriteUInt8(ActionType);
    bw.WriteVarUInt(static_cast<uint32_t>(Entries.size()));

    if (ActionType == PlayerListActionRemove) {
        for (const auto& entry : Entries) {
            bw.Write(entry.UUID.data(), 16);
        }
    }
    // Add 模式的序列化过于复杂，省略

    return bw.vect();
}

std::vector<std::string> PlayerList::GetPlayerNames() const
{
    std::vector<std::string> names;
    names.reserve(Entries.size());
    for (const auto& entry : Entries) {
        names.push_back(entry.Username);
    }
    return names;
}
