#pragma once
#include "PacketBase.h"
class Disconnect :
    public PacketBase
{
public:
	unsigned char ID() override;
	void Deserializ(std::vector<unsigned char> pack) override;
	string message;
};

