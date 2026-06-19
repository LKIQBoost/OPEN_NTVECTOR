#pragma once
#include "PacketBase.h"
class RequestNetworkSettings : public PacketBase
{
public:
	unsigned char ID() override;
	std::vector<unsigned char> Serializ() override;
	uint32_t ProtocolVersion;
};

