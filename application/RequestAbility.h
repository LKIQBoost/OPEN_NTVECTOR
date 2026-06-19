#pragma once
#include "PacketBase.h"
class RequestAbility :
    public PacketBase
{
public:
    virtual unsigned char ID() override
    {
        return IDRequestAbility;
    }
    virtual std::vector<unsigned char> Serializ() override
    {
        BinaryWriter writer(5);
        writer.WriteUInt8(ID());
        writer.WriteVarUInt32(unknown);
        writer.WriteVarUInt32(unknown2);
        writer.WriteVarUInt32(unknown3);
        return writer.vect();
    }
    int unknown;
    int unknown2;
    int unknown3;
};

