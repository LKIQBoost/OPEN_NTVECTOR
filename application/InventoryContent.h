#pragma once
#include "PacketBase.h"
#include "ProtocolRead.h"
#include <vector>

// Simplified representation of a bedrock item stack (network format).
struct SlotItem {
    int32_t  NetworkID = 0;          // 0 = air
    uint16_t Count = 0;
    uint32_t Metadata = 0;           // plain unsigned varint
    bool     HasStackID = false;     // stack network id present
    int32_t  StackID = 0;
    int32_t  BlockRuntimeID = 0;
    std::vector<uint8_t> Extra;      // raw encapsulated "extra" payload bytes

    bool IsAir() const { return NetworkID == 0 || NetworkID == -1; }
};

namespace pkt_io {

    // Item stack, network format (minecraft-data bedrock 1.21.120 "Item"):
    //   zigzag32 network_id          (0 = air; nothing follows for air)
    //   lu16     count
    //   varint   metadata
    //   u8       has_stack_id -> zigzag32 stack_id (if != 0)
    //   zigzag32 block_runtime_id
    //   varint   extra length + raw extra payload (nbt / can_place_on / can_destroy...)
    static inline void ReadItem(BinaryReader& br, SlotItem& out) {
        out = SlotItem();

        out.NetworkID = ReadZigZag32(br);
        if (out.IsAir()) {
            return;
        }

        out.Count = br.ReadUInt16();
        out.Metadata = br.ReadVarUInt();

        uint8_t hasStackId = br.ReadUInt8();
        if (hasStackId != 0) {
            out.HasStackID = true;
            out.StackID = ReadZigZag32(br);
        }

        out.BlockRuntimeID = ReadZigZag32(br);

        uint32_t extraLen = br.ReadVarUInt();
        if (extraLen > 0 && extraLen < (1 << 20)) {
            const uint8_t* p = reinterpret_cast<const uint8_t*>(br.Read(static_cast<size_t>(extraLen)));
            if (p) {
                out.Extra.assign(p, p + extraLen);
            }
        }
    }

}

// InventoryContentPacket (0x31) - server -> client
//
// Full slot list of a container / inventory window. Sent right after
// ContainerOpenPacket (46) with the window's contents.
//
// Wire layout verified against minecraft-data bedrock 1.21.120:
//   varint  window_id        (WindowIDVarint)
//   varint  slot count
//   Item[]  input            (slots)
//   u8      container_id     (FullContainerName.container_id)
//   option  dynamic_container_id  (u8 presence + u32 if present)
//   Item    storage_item
class InventoryContent : public PacketBase
{
public:
    unsigned char ID() override
    {
        return IDInventoryContent;
    }

    void Deserializ(std::vector<unsigned char> pack) override
    {
        BinaryReader br(pack.data(), static_cast<int>(pack.size()));

        WindowID = static_cast<int32_t>(br.ReadVarUInt());

        uint64_t count = br.ReadVarUInt();
        Slots.clear();
        if (count > 512) count = 512;               // sanity cap
        Slots.reserve(static_cast<size_t>(count));
        for (uint64_t i = 0; i < count; ++i) {
            SlotItem item;
            pkt_io::ReadItem(br, item);
            Slots.push_back(std::move(item));
        }

        // FullContainerName
        ContainerID = br.ReadUInt8();
        uint8_t hasDynamic = br.ReadUInt8();
        if (hasDynamic != 0) {
            HasDynamicContainerID = true;
            DynamicContainerID = br.ReadUInt32();
        }

        // storage item (the item that owns this storage, e.g. a bundle)
        pkt_io::ReadItem(br, StorageItem);
    }

    int32_t  WindowID = 0;
    std::vector<SlotItem> Slots;

    uint8_t  ContainerID = 0;
    bool     HasDynamicContainerID = false;
    uint32_t DynamicContainerID = 0;
    SlotItem StorageItem;
};
