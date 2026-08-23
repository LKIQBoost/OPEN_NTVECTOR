#pragma once
#include "PacketBase.h"
#include "ProtocolRead.h"

// ContainerOpenPacket (0x2E) - server -> client
//
// Sent when a container window is opened for the player.
//
// Wire layout verified against minecraft-data bedrock 1.21.120:
//   i8       window_id          (WindowID enum, e.g. 0 = inventory, 2+ = containers)
//   i8       window_type        (WindowType enum, e.g. 0 = container, 2 = furnace)
//   blockpos coordinates        (x zigzag32, y varint, z zigzag32)
//   zigzag64 runtime_entity_id  (for entity containers: horse, minecart chest...)
struct ContainerOpenInfo {
    int8_t  WindowID = 0;
    int8_t  WindowType = 0;         // WindowType enum value
    BlockPos Position;
    int64_t EntityUniqueID = 0;
};

class ContainerOpen : public PacketBase
{
public:
    unsigned char ID() override
    {
        return IDContainerOpen;
    }

    void Deserializ(std::vector<unsigned char> pack) override
    {
        BinaryReader br(pack.data(), static_cast<int>(pack.size()));

        WindowID = br.ReadInt8();
        WindowType = br.ReadInt8();
        Position = pkt_io::ReadBlockPos(br);
        EntityUniqueID = pkt_io::ReadZigZag64(br);
    }

    ContainerOpenInfo ToInfo() const
    {
        ContainerOpenInfo info;
        info.WindowID = WindowID;
        info.WindowType = WindowType;
        info.Position = Position;
        info.EntityUniqueID = EntityUniqueID;
        return info;
    }

    int8_t  WindowID = 0;
    int8_t  WindowType = 0;
    BlockPos Position;
    int64_t EntityUniqueID = 0;
};
