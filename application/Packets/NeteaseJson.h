#pragma once
#include "PacketBase.h"
class NeteaseJson :
    public PacketBase
{
public:
	unsigned char ID() override;
	std::vector<unsigned char> Serializ() override;
	bool Unknow;
	string Json;
};

