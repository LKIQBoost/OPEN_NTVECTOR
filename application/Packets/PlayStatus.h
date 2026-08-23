#pragma once
#include "PacketBase.h"
class PlayStatus :
    public PacketBase
{
public:
	unsigned char ID() override;
	void Deserializ(std::vector<unsigned char> pack) override;
	int32_t Status;
};

