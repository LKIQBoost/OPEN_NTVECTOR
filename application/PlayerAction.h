#pragma once
#include "PacketBase.h"
class PlayerAction :
    public PacketBase
{
public:
    virtual unsigned char ID() override
    {
        return IDPlayerAction;
    }
    virtual std::vector<unsigned char> Serializ() override
    {
        BinaryWriter bw(256);
        bw.WriteUInt8(ID());
        bw.WriteVarUInt64(EntityRuntimeID);
        bw.WriteVarUInt(ActionType);
        bw.WriteVarUInt(BlockPosition.x);
        bw.WriteVarUInt(BlockPosition.y);
        bw.WriteVarUInt(BlockPosition.z);
        bw.WriteVarUInt(ResultPosition.x);
        bw.WriteVarUInt(ResultPosition.y);
        bw.WriteVarUInt(ResultPosition.z);
        bw.WriteVarUInt(BlockFace);
        return bw.vect();
    }
    // EntityRuntimeID is the runtime ID of the player. The runtime ID is unique for each world session, and
    // entities are generally identified in packets using this runtime ID.
    uint64_t EntityRuntimeID;
        // ActionType is the ID of the action that was executed by the player. It is one of the constants that may
        // be found in protocol/player.go.
    int ActionType;
        // BlockPosition is the position of the target block, if the action with the ActionType set concerned a
        // block. If that is not the case, the block position will be zero.
    Vec3d BlockPosition;
        // ResultPosition is the position of the action's result. When a UseItemOn action is sent, this is the position of
        // the block clicked, but when a block is placed, this is the position at which the block will be placed.
    Vec3d ResultPosition;
        // BlockFace is the face of the target block that was touched. If the action with the ActionType set
        // concerned a block. If not, the face is always 0.
    int BlockFace;
};

