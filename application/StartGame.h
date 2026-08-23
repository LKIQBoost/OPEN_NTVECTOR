#pragma once
#include "PacketBase.h"
class StartGame :
    public PacketBase
{
public:
	unsigned char ID() override;
	void Deserializ(std::vector<unsigned char> pack) override;
	__int64 playerEntityID;
	__int64 EntityRuntimeID;
	int gamemode;
	Vec3 PlayerPosition;
	float Pitch;
	float Yaw;
};

