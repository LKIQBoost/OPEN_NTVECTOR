#pragma once

#include "PacketBase.h"
#include <array>
#include <cstdint>
#include <string>
#include <vector>

enum class PlayerListPacketType : uint8_t {
    Add = 0,
    Remove = 1,
};

// Compatibility constants retained for existing callers.
constexpr uint8_t PlayerListActionAdd = static_cast<uint8_t>(PlayerListPacketType::Add);
constexpr uint8_t PlayerListActionRemove = static_cast<uint8_t>(PlayerListPacketType::Remove);

struct PlayerSkinImage {
    int32_t Width = 0;
    int32_t Height = 0;
    std::vector<uint8_t> Data;
};

struct PlayerSkinAnimation {
    PlayerSkinImage SkinImage;
    int32_t AnimationType = 0;
    float AnimationFrames = 0.0f;
    float ExpressionType = 0.0f;
};

struct PlayerPersonaPiece {
    std::string PieceID;
    std::string PieceType;
    std::string PackID;
    bool IsDefaultPiece = false;
    std::string ProductID;
};

struct PlayerPieceTintColors {
    std::string PieceType;
    std::vector<std::string> Colors;
};

struct PlayerSkin {
    std::string SkinID;
    std::string PlayFabID;
    std::string SkinResourcePatch;

    PlayerSkinImage SkinImage;
    std::vector<PlayerSkinAnimation> Animations;
    PlayerSkinImage CapeImage;
    std::string GeometryData;
    std::string GeometryDataVersion;
    std::string AnimationData;
    std::string CapeID;
    std::string FullID;
    std::string ArmSize;
    std::string SkinColour;
    std::vector<PlayerPersonaPiece> PersonaPieceRecords;
    std::vector<PlayerPieceTintColors> PieceTintColorRecords;
    bool PremiumSkin = false;
    bool PersonaSkin = false;
    bool PersonaCapeOnClassicSkin = false;
    bool PrimaryUser = false;
    bool OverrideAppearance = false;

    // TrustedSkinFlag is internal to the game skin object and is not on the wire.
    bool Trusted = false;

    // Compatibility views used by the existing Python event conversion code.
    int32_t SkinImageWidth = 0;
    int32_t SkinImageHeight = 0;
    std::vector<uint8_t> SkinData;
    uint32_t AnimationCount = 0;
    int32_t CapeImageWidth = 0;
    int32_t CapeImageHeight = 0;
    std::string CapeData;
    std::vector<uint8_t> SkinGeometry;
    std::vector<uint8_t> GeometryDataEngineVersion;
    std::vector<std::string> PersonaPieces;
    std::vector<std::string> PieceTintColours;
};

struct PlayerListEntry {
    std::array<uint8_t, 16> UUID{};
    int64_t EntityUniqueID = 0;
    std::string Username;
    std::string XUID;
    std::string PlatformChatID;
    int32_t BuildPlatform = 0;
    PlayerSkin Skin;
    bool Teacher = false;
    bool Host = false;
    bool SubClient = false;
    int32_t PlayerColor = 0;

    // Deprecated compatibility name. It mirrors PlayerColor on decode.
    int32_t Unknown = 0;
};

class PlayerListPacket : public PacketBase
{
public:
    unsigned char ID() override;
    void Deserializ(std::vector<unsigned char> pack) override;
    std::vector<unsigned char> Serializ() override;

    PlayerListPacketType Action = PlayerListPacketType::Add;
    uint8_t ActionType = PlayerListActionAdd;
    std::vector<PlayerListEntry> Entries;
    std::vector<bool> Verified;

    std::vector<std::string> GetPlayerNames() const;
    bool IsAddAction() const { return Action == PlayerListPacketType::Add; }
    bool IsRemoveAction() const { return Action == PlayerListPacketType::Remove; }
};

// Existing code uses PlayerList; keep it as an alias of the complete packet.
using PlayerList = PlayerListPacket;
