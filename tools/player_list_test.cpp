#include "Packets/PlayerListPacket.h"

#include <cassert>
#include <cstdint>
#include <iostream>

namespace {

PlayerSkinImage image(int32_t width, int32_t height, std::initializer_list<uint8_t> data)
{
    PlayerSkinImage result;
    result.Width = width;
    result.Height = height;
    result.Data.assign(data.begin(), data.end());
    return result;
}

PlayerListPacket makeAddPacket()
{
    PlayerListPacket packet;
    packet.Action = PlayerListPacketType::Add;
    packet.ActionType = PlayerListActionAdd;

    PlayerListEntry entry;
    for (size_t i = 0; i < entry.UUID.size(); ++i) entry.UUID[i] = static_cast<uint8_t>(i);
    entry.EntityUniqueID = -123;
    entry.Username = "ProtocolTest";
    entry.XUID = "xuid";
    entry.PlatformChatID = "platform";
    entry.BuildPlatform = -7;
    entry.Teacher = true;
    entry.Host = false;
    entry.SubClient = true;
    entry.PlayerColor = 0x12345678;

    entry.Skin.SkinID = "skin-id";
    entry.Skin.PlayFabID = "playfab-id";
    entry.Skin.SkinResourcePatch = "resource-patch";
    entry.Skin.SkinImage = image(64, 64, {1, 2, 3});
    entry.Skin.Animations.push_back({image(8, 8, {4}), 2, 1.5f, 0.25f});
    entry.Skin.CapeImage = image(16, 8, {5, 6});
    entry.Skin.GeometryData = "geometry";
    entry.Skin.GeometryDataVersion = "geometry-version";
    entry.Skin.AnimationData = "animation";
    entry.Skin.CapeID = "cape-id";
    entry.Skin.FullID = "full-id";
    entry.Skin.ArmSize = "wide";
    entry.Skin.SkinColour = "#ffffff";
    entry.Skin.PersonaPieceRecords.push_back({"piece-id", "piece-type", "pack-id", true, "product-id"});
    entry.Skin.PieceTintColorRecords.push_back({"piece-type", {"#111111", "#222222"}});
    entry.Skin.PremiumSkin = true;
    entry.Skin.PersonaSkin = true;
    entry.Skin.PersonaCapeOnClassicSkin = true;
    entry.Skin.PrimaryUser = true;
    entry.Skin.OverrideAppearance = true;

    packet.Entries.push_back(entry);
    packet.Verified.push_back(true);
    return packet;
}

void assertAddPacket(const PlayerListPacket& packet)
{
    assert(packet.Action == PlayerListPacketType::Add);
    assert(packet.ActionType == PlayerListActionAdd);
    assert(packet.Entries.size() == 1);
    assert(packet.Verified.size() == 1 && packet.Verified[0]);

    const auto& entry = packet.Entries[0];
    assert(entry.EntityUniqueID == -123);
    assert(entry.Username == "ProtocolTest");
    assert(entry.BuildPlatform == -7);
    assert(entry.PlayerColor == 0x12345678);
    assert(entry.Unknown == entry.PlayerColor);
    assert(entry.Skin.SkinImage.Width == 64);
    assert(entry.Skin.SkinImage.Data == std::vector<uint8_t>({1, 2, 3}));
    assert(entry.Skin.Animations.size() == 1);
    assert(entry.Skin.Animations[0].AnimationType == 2);
    assert(entry.Skin.Animations[0].AnimationFrames == 1.5f);
    assert(entry.Skin.CapeImage.Data == std::vector<uint8_t>({5, 6}));
    assert(entry.Skin.PersonaPieceRecords.size() == 1);
    assert(entry.Skin.PieceTintColorRecords.size() == 1);
    assert(entry.Skin.PieceTintColorRecords[0].Colors.size() == 2);
    assert(entry.Skin.OverrideAppearance);
}

} // namespace

int main()
{
    PlayerListPacket original = makeAddPacket();
    const std::vector<unsigned char> addBytes = original.Serializ();

    PlayerListPacket parsedAdd;
    parsedAdd.Deserializ(addBytes);
    assertAddPacket(parsedAdd);
    assert(parsedAdd.Serializ() == addBytes);

    PlayerListPacket remove;
    remove.Action = PlayerListPacketType::Remove;
    remove.ActionType = PlayerListActionRemove;
    remove.Entries.push_back(original.Entries[0]);
    const std::vector<unsigned char> removeBytes = remove.Serializ();

    PlayerListPacket parsedRemove;
    parsedRemove.Deserializ(removeBytes);
    assert(parsedRemove.IsRemoveAction());
    assert(parsedRemove.Entries.size() == 1);
    assert(parsedRemove.Entries[0].UUID == original.Entries[0].UUID);
    assert(parsedRemove.Entries[0].Username.empty());
    assert(parsedRemove.Serializ() == removeBytes);

    std::cout << "PlayerListPacket protocol round-trip passed\n";
    return 0;
}
