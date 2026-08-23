#pragma once
#include "PacketBase.h"
class AddPlayer :
    public PacketBase
{
public:
    unsigned char ID() override;
    void Deserializ(std::vector<unsigned char> pack) override;
    std::vector<unsigned char> Serializ() override;
    std::string uuid;
    std::string userName;
    uint64_t EntityRuntimeID;
    std::string PlatformChatID;
    Vec3 Position;
    Vec3 Velocity;
    float Pitch;
    float Yaw;
    float HeadYaw;
    ItemInstance HeldItem;
    int GameType;
};

