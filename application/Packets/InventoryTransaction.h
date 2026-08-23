#pragma once
#include "PacketBase.h"
#include "CommandBlockUpdate.h"   // BlockPos

// InventoryTransactionPacket (0x1E) - client -> server
//
// Used to open a container: we send an "item_use" transaction with
// action_type = 0 (click_block) targeting the container block.
//
// Wire layout verified against minecraft-data bedrock 1.21.120:
//   zigzag32 legacy_request_id     = -1        (modern item-stack-request flow)
//   varint   legacy_transactions   count       = 0
//   varint   transaction_type      = 2 (item_use)
//   varint   actions count                    = 0
//   --- TransactionUseItem ---
//   varint   action_type           = 0 (click_block)
//   varint   trigger_type          = 0 (unknown)
//   blockpos block_position        (x zigzag32, y varint, z zigzag32)
//   zigzag32 face                  = 1
//   zigzag32 hotbar_slot           = 0
//   item     held_item             (air: single zigzag32 0)
//   vec3f    player_pos
//   vec3f    click_pos             (click point on the block face)
//   varint   block_runtime_id      = 0
//   varint   client_prediction     = 0 (failure)
class InventoryTransaction : public PacketBase
{
public:
    unsigned char ID() override
    {
        return IDInventoryTransaction;
    }

    std::vector<unsigned char> Serializ() override
    {
        BinaryWriter bw(256);

        // packet id
        bw.WriteVarUInt32(ID());

        // legacy_request_id = -1 (zigzag32)
        bw.WriteVarInt(-1);

        // legacy_transactions: 0 entries (varint count)
        bw.WriteVarUInt32(0);

        // transaction_type = 2 (item_use) - plain unsigned varint
        bw.WriteVarUInt32(2);

        // actions: 0 entries (varint count)
        bw.WriteVarUInt32(0);

        // --- TransactionUseItem ---
        // action_type = 0 (click_block)
        bw.WriteVarUInt32(0);

        // trigger_type = 0 (unknown)
        bw.WriteVarUInt32(0);

        // block position (x/z zigzag32, y unsigned varint - BlockCoordinates)
        bw.WriteVarInt(BlockPosition.x);
        bw.WriteVarUInt(static_cast<uint32_t>(BlockPosition.y));
        bw.WriteVarInt(BlockPosition.z);

        // face (zigzag32, 1 = up)
        bw.WriteVarInt(BlockFace);

        // hotbar slot (zigzag32)
        bw.WriteVarInt(HotbarSlot);

        // held item: air (network_id 0, one zigzag byte)
        bw.WriteVarInt(0);

        // player position
        bw.WriteVec3(PlayerPosition);

        // click point on the block face
        bw.WriteVec3(WorldPosition);

        // block runtime id (varint, 0 = not validated)
        bw.WriteVarUInt32(0);

        // client_prediction = 0 (failure)
        bw.WriteVarUInt32(0);

        return bw.vect();
    }

    BlockPos BlockPosition;          // container block position
    int      BlockFace    = 1;       // face clicked (1 = up)
    int      HotbarSlot   = 0;       // hotbar slot used for the click
    Vec3     PlayerPosition{};       // player position at click time
    Vec3     WorldPosition{};        // exact click point on the block face
};
