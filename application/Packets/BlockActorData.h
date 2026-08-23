#pragma once
#include "NBT/Tag.h"
#include "PacketBase.h"
#include "ProtocolRead.h"
#include <map>
#include <string>
#include <vector>

// BlockActorDataPacket (0x38) - server -> client
//
// Pushes the NBT data of a block entity (sign text, command block command,
// container custom name, ...) when it changes. The same packet is also sent
// by the client when a player edits a sign.
//
// Wire layout verified against minecraft-data bedrock 1.21.120:
//   blockpos position       (BlockCoordinates: x zigzag32, y varint, z zigzag32)
//   nbt      named tag      (little-endian NBT with varint lengths, rest of packet)
class BlockActorData : public PacketBase
{
public:
    unsigned char ID() override
    {
        return IDBlockActorData;
    }

    void Deserializ(std::vector<unsigned char> pack) override;

    // Compatibility helper for existing callers that consume dotted strings.
    static bool FlattenNbt(const uint8_t* data, size_t len, std::map<std::string, std::string>& out);

    BlockPos Position;
    std::vector<uint8_t> RawNBT;
    nbt::Tag NBT;
    std::string NBTRootName;
    std::string NBTError;
    bool NBTParsed = false;
    std::map<std::string, std::string> NBTFields;
};
