#include "PlayerList.h"

#include <cstring>
#include <stdexcept>

namespace {

int32_t readListCount(BinaryReader& br, const char* fieldName)
{
    const int32_t count = br.ReadInt32();
    if (count < 0) {
        throw std::runtime_error(std::string(fieldName) + " count is negative");
    }
    return count;
}

int64_t readZigZag64(BinaryReader& br)
{
    const uint64_t encoded = static_cast<uint64_t>(br.ReadVarInt64());
    return static_cast<int64_t>((encoded >> 1) ^ (0 - (encoded & 1)));
}

void readSkinImage(BinaryReader& br, PlayerSkinImage& image)
{
    image.Width = br.ReadInt32();
    image.Height = br.ReadInt32();
    br.ReadByteSlice(image.Data);
}

void writeSkinImage(BinaryWriter& bw, const PlayerSkinImage& image)
{
    bw.WriteInt32(image.Width);
    bw.WriteInt32(image.Height);
    bw.WriteByteSlice(image.Data.data(), image.Data.size());
}

void readSkin(BinaryReader& br, PlayerSkin& skin)
{
    br.ReadStringUTF(skin.SkinID);
    br.ReadStringUTF(skin.PlayFabID);
    br.ReadStringUTF(skin.SkinResourcePatch);

    readSkinImage(br, skin.SkinImage);
    skin.SkinImageWidth = skin.SkinImage.Width;
    skin.SkinImageHeight = skin.SkinImage.Height;
    skin.SkinData = skin.SkinImage.Data;

    const int32_t animationCount = readListCount(br, "skin animation");
    skin.Animations.clear();
    skin.Animations.reserve(static_cast<size_t>(animationCount));
    for (int32_t i = 0; i < animationCount; ++i) {
        PlayerSkinAnimation animation;
        readSkinImage(br, animation.SkinImage);
        animation.AnimationType = br.ReadInt32();
        animation.AnimationFrames = br.ReadFloat();
        animation.ExpressionType = br.ReadFloat();
        skin.Animations.push_back(std::move(animation));
    }
    skin.AnimationCount = static_cast<uint32_t>(skin.Animations.size());

    readSkinImage(br, skin.CapeImage);
    skin.CapeImageWidth = skin.CapeImage.Width;
    skin.CapeImageHeight = skin.CapeImage.Height;
    skin.CapeData.assign(reinterpret_cast<const char*>(skin.CapeImage.Data.data()), skin.CapeImage.Data.size());

    br.ReadStringUTF(skin.GeometryData);
    br.ReadStringUTF(skin.GeometryDataVersion);
    br.ReadStringUTF(skin.AnimationData);
    br.ReadStringUTF(skin.CapeID);
    br.ReadStringUTF(skin.FullID);
    br.ReadStringUTF(skin.ArmSize);
    br.ReadStringUTF(skin.SkinColour);

    skin.SkinGeometry.assign(skin.GeometryData.begin(), skin.GeometryData.end());
    skin.GeometryDataEngineVersion.assign(skin.GeometryDataVersion.begin(), skin.GeometryDataVersion.end());

    const int32_t personaCount = readListCount(br, "persona piece");
    skin.PersonaPieceRecords.clear();
    skin.PersonaPieceRecords.reserve(static_cast<size_t>(personaCount));
    skin.PersonaPieces.clear();
    skin.PersonaPieces.reserve(static_cast<size_t>(personaCount));
    for (int32_t i = 0; i < personaCount; ++i) {
        PlayerPersonaPiece piece;
        br.ReadStringUTF(piece.PieceID);
        br.ReadStringUTF(piece.PieceType);
        br.ReadStringUTF(piece.PackID);
        br.ReadBool(piece.IsDefaultPiece);
        br.ReadStringUTF(piece.ProductID);
        skin.PersonaPieces.push_back(piece.PieceID);
        skin.PersonaPieceRecords.push_back(std::move(piece));
    }

    const int32_t tintGroupCount = readListCount(br, "piece tint group");
    skin.PieceTintColorRecords.clear();
    skin.PieceTintColorRecords.reserve(static_cast<size_t>(tintGroupCount));
    skin.PieceTintColours.clear();
    for (int32_t i = 0; i < tintGroupCount; ++i) {
        PlayerPieceTintColors group;
        br.ReadStringUTF(group.PieceType);
        const int32_t colorCount = readListCount(br, "piece tint color");
        group.Colors.reserve(static_cast<size_t>(colorCount));
        for (int32_t color = 0; color < colorCount; ++color) {
            std::string value;
            br.ReadStringUTF(value);
            skin.PieceTintColours.push_back(value);
            group.Colors.push_back(std::move(value));
        }
        skin.PieceTintColorRecords.push_back(std::move(group));
    }

    br.ReadBool(skin.PremiumSkin);
    br.ReadBool(skin.PersonaSkin);
    br.ReadBool(skin.PersonaCapeOnClassicSkin);
    br.ReadBool(skin.PrimaryUser);
    br.ReadBool(skin.OverrideAppearance);
}

void writeSkin(BinaryWriter& bw, const PlayerSkin& skin)
{
    bw.WriteStringUTF(skin.SkinID);
    bw.WriteStringUTF(skin.PlayFabID);
    bw.WriteStringUTF(skin.SkinResourcePatch);

    PlayerSkinImage skinImage = skin.SkinImage;
    if (skinImage.Width == 0 && skinImage.Height == 0 && skinImage.Data.empty()) {
        skinImage.Width = skin.SkinImageWidth;
        skinImage.Height = skin.SkinImageHeight;
        skinImage.Data = skin.SkinData;
    }
    writeSkinImage(bw, skinImage);

    bw.WriteInt32(static_cast<int32_t>(skin.Animations.size()));
    for (const auto& animation : skin.Animations) {
        writeSkinImage(bw, animation.SkinImage);
        bw.WriteInt32(animation.AnimationType);
        bw.WriteFloat(animation.AnimationFrames);
        bw.WriteFloat(animation.ExpressionType);
    }

    PlayerSkinImage capeImage = skin.CapeImage;
    if (capeImage.Width == 0 && capeImage.Height == 0 && capeImage.Data.empty()) {
        capeImage.Width = skin.CapeImageWidth;
        capeImage.Height = skin.CapeImageHeight;
        capeImage.Data.assign(skin.CapeData.begin(), skin.CapeData.end());
    }
    writeSkinImage(bw, capeImage);

    std::string geometryData = skin.GeometryData;
    if (geometryData.empty() && !skin.SkinGeometry.empty()) {
        geometryData.assign(skin.SkinGeometry.begin(), skin.SkinGeometry.end());
    }
    std::string geometryVersion = skin.GeometryDataVersion;
    if (geometryVersion.empty() && !skin.GeometryDataEngineVersion.empty()) {
        geometryVersion.assign(skin.GeometryDataEngineVersion.begin(), skin.GeometryDataEngineVersion.end());
    }
    bw.WriteStringUTF(geometryData);
    bw.WriteStringUTF(geometryVersion);
    bw.WriteStringUTF(skin.AnimationData);
    bw.WriteStringUTF(skin.CapeID);
    bw.WriteStringUTF(skin.FullID);
    bw.WriteStringUTF(skin.ArmSize);
    bw.WriteStringUTF(skin.SkinColour);

    bw.WriteInt32(static_cast<int32_t>(skin.PersonaPieceRecords.size()));
    for (const auto& piece : skin.PersonaPieceRecords) {
        bw.WriteStringUTF(piece.PieceID);
        bw.WriteStringUTF(piece.PieceType);
        bw.WriteStringUTF(piece.PackID);
        bw.WriteBool(piece.IsDefaultPiece);
        bw.WriteStringUTF(piece.ProductID);
    }

    bw.WriteInt32(static_cast<int32_t>(skin.PieceTintColorRecords.size()));
    for (const auto& group : skin.PieceTintColorRecords) {
        bw.WriteStringUTF(group.PieceType);
        bw.WriteInt32(static_cast<int32_t>(group.Colors.size()));
        for (const auto& color : group.Colors) bw.WriteStringUTF(color);
    }

    bw.WriteBool(skin.PremiumSkin);
    bw.WriteBool(skin.PersonaSkin);
    bw.WriteBool(skin.PersonaCapeOnClassicSkin);
    bw.WriteBool(skin.PrimaryUser);
    bw.WriteBool(skin.OverrideAppearance);
}

void readEntry(BinaryReader& br, PlayerListEntry& entry)
{
    const void* uuidData = br.Read(entry.UUID.size());
    if (uuidData) std::memcpy(entry.UUID.data(), uuidData, entry.UUID.size());

    entry.EntityUniqueID = readZigZag64(br);
    br.ReadStringUTF(entry.Username);
    br.ReadStringUTF(entry.XUID);
    br.ReadStringUTF(entry.PlatformChatID);
    entry.BuildPlatform = br.ReadInt32();
    readSkin(br, entry.Skin);
    br.ReadBool(entry.Teacher);
    br.ReadBool(entry.Host);
    br.ReadBool(entry.SubClient);
    entry.PlayerColor = br.ReadInt32();
    entry.Unknown = entry.PlayerColor;
}

void writeEntry(BinaryWriter& bw, const PlayerListEntry& entry)
{
    bw.Write(entry.UUID.data(), entry.UUID.size());
    bw.WriteVarInt64(entry.EntityUniqueID);
    bw.WriteStringUTF(entry.Username);
    bw.WriteStringUTF(entry.XUID);
    bw.WriteStringUTF(entry.PlatformChatID);
    bw.WriteInt32(entry.BuildPlatform);
    writeSkin(bw, entry.Skin);
    bw.WriteBool(entry.Teacher);
    bw.WriteBool(entry.Host);
    bw.WriteBool(entry.SubClient);
    const int32_t playerColor = entry.PlayerColor != 0 ? entry.PlayerColor : entry.Unknown;
    bw.WriteInt32(playerColor);
}

} // namespace

unsigned char PlayerListPacket::ID()
{
    return IDPlayerList;
}

void PlayerListPacket::Deserializ(std::vector<unsigned char> pack)
{
    BinaryReader br(pack.data(), static_cast<int>(pack.size()));
    ActionType = br.ReadUInt8();
    if (ActionType > PlayerListActionRemove) {
        throw std::runtime_error("invalid PlayerList action");
    }
    Action = static_cast<PlayerListPacketType>(ActionType);

    const uint32_t entryCount = br.ReadVarUInt();
    Entries.clear();
    Entries.reserve(entryCount);
    Verified.clear();

    if (IsAddAction()) {
        for (uint32_t i = 0; i < entryCount; ++i) {
            PlayerListEntry entry;
            readEntry(br, entry);
            Entries.push_back(std::move(entry));
        }
        Verified.reserve(entryCount);
        for (uint32_t i = 0; i < entryCount; ++i) {
            bool verified = false;
            br.ReadBool(verified);
            Verified.push_back(verified);
        }
    } else {
        for (uint32_t i = 0; i < entryCount; ++i) {
            PlayerListEntry entry;
            const void* uuidData = br.Read(entry.UUID.size());
            if (uuidData) std::memcpy(entry.UUID.data(), uuidData, entry.UUID.size());
            Entries.push_back(std::move(entry));
        }
    }
}

std::vector<unsigned char> PlayerListPacket::Serializ()
{
    BinaryWriter bw(1024);
    uint8_t actionValue = ActionType;
    if (actionValue == PlayerListActionAdd && Action == PlayerListPacketType::Remove) {
        actionValue = PlayerListActionRemove;
    }
    if (actionValue > PlayerListActionRemove) throw std::runtime_error("invalid PlayerList action");
    const PlayerListPacketType action = static_cast<PlayerListPacketType>(actionValue);
    Action = action;
    ActionType = actionValue;

    bw.WriteUInt8(actionValue);
    bw.WriteVarUInt(static_cast<uint32_t>(Entries.size()));
    if (action == PlayerListPacketType::Add) {
        for (const auto& entry : Entries) writeEntry(bw, entry);
        for (size_t i = 0; i < Entries.size(); ++i) {
            bw.WriteBool(i < Verified.size() ? Verified[i] : false);
        }
    } else {
        for (const auto& entry : Entries) bw.Write(entry.UUID.data(), entry.UUID.size());
    }
    return bw.vect();
}

std::vector<std::string> PlayerListPacket::GetPlayerNames() const
{
    std::vector<std::string> names;
    names.reserve(Entries.size());
    for (const auto& entry : Entries) names.push_back(entry.Username);
    return names;
}
