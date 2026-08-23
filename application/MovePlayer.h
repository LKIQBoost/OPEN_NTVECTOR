#pragma once
#include "PacketBase.h"
class MovePlayer : public PacketBase
{
public:
    virtual unsigned char ID() override
    {
        return IDMovePlayer;
    }
    virtual std::vector<unsigned char> Serializ() override
    {
        BinaryWriter bw(256);
        bw.WriteUInt8(ID());
        bw.WriteVarInt64(EntityRuntimeID);          // varint64，与 StartGame 一致
        bw.WriteFloat(Position.x);
        bw.WriteFloat(Position.y);
        bw.WriteFloat(Position.z);
        bw.WriteFloat(Pitch);
        bw.WriteFloat(Yaw);
        bw.WriteFloat(HeadYaw);
        bw.WriteUInt8(Mode);                        // 0 normal, 1 reset, 2 teleport, 3 rotation
        bw.WriteBool(OnGround);
        bw.WriteVarInt((int)RiddenRuntimeID);
        if (Mode == 2) {                            // teleport 附加字段 (1.21.120)
            bw.WriteInt32(Cause);                   // teleport cause (li32)
            bw.WriteInt32(SourceEntityType);        // LegacyEntityType (li32)
        }
        bw.WriteVarInt64(Tick);
        return bw.vect();
    }
    virtual void Deserializ(std::vector<unsigned char> pack) override
    {
        BinaryReader br(pack.data(), pack.size());
        EntityRuntimeID = br.ReadVarInt64();  // varint64，与 StartGame::EntityRuntimeID 保持一致，避免大 ID 比较失败
        Position = br.ReadVec3();
        Pitch = br.ReadFloat();
        Yaw = br.ReadFloat();
        HeadYaw = br.ReadFloat();
    }
    __int64 EntityRuntimeID;
    Vec3 Position;
    float Pitch;
    float Yaw;
    float HeadYaw;
    // 1.21.120 新增字段
    unsigned char Mode = 0;          // 0 normal, 1 reset, 2 teleport, 3 rotation
    bool OnGround = true;
    __int64 RiddenRuntimeID = 0;
    int Cause = 0;                   // teleport cause: 0 unknown 1 projectile 2 chorus_fruit 3 command 4 behavior
    int SourceEntityType = 0;        // LegacyEntityType (li32), 0 = none
    __int64 Tick = 0;
};

