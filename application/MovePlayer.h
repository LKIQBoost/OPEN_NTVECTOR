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
        bw.WriteVarInt(EntityRuntimeID);
        bw.WriteFloat(Position.x);
        bw.WriteFloat(Position.y);
        bw.WriteFloat(Position.z);
        bw.WriteFloat(Pitch);
        bw.WriteFloat(Yaw);
        bw.WriteFloat(HeadYaw);
        return bw.vect();
    }
    virtual void Deserializ(std::vector<unsigned char> pack) override
    {
        BinaryReader br(pack.data(), pack.size());
        EntityRuntimeID = br.ReadVarInt();
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
};

