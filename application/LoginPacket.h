#pragma once
#include "PacketBase.h"
class LoginPacket :
    public PacketBase
{
public:
	unsigned char ID() override;
	std::vector<unsigned char> Serializ() override;
	uint32_t ProtocolVersion;
	std::string chain;
	std::string skindata;
};

