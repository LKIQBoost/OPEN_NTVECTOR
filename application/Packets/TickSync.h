#pragma once
#include "PacketBase.h"
class TickSync : public PacketBase
{
public:
    virtual unsigned char ID() override {
        return IDTickSync;
    }
    virtual std::vector<unsigned char> Serializ() override {
        BinaryWriter bw(25);
        bw.WriteUInt8(ID());
        bw.WriteVarInt64(ClientRequestTick);
        bw.WriteVarInt64(ServerReceptionTick);
        return bw.vect();
    }
    virtual void Deserializ(std::vector<unsigned char> pack) override {
        BinaryReader br(pack.data(), static_cast<int>(pack.size()));
        ClientRequestTick = br.ReadVarInt64();
        ServerReceptionTick = br.ReadVarInt64();
    }
    __int64 ClientRequestTick;
    __int64 ServerReceptionTick;
};
