#pragma once
#include "PacketBase.h"
class ClientCacheStatus :
    public PacketBase
{
public:
	unsigned char ID() override;
	std::vector<unsigned char> Serializ() override;
	bool Enabled;
	bool Unknow;
};

