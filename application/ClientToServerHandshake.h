#pragma once
#include "PacketBase.h"
class ClientToServerHandshake :
    public PacketBase
{
	unsigned char ID() override;
	std::vector<unsigned char> Serializ() override;
};

