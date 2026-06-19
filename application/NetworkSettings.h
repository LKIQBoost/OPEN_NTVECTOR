#pragma once
#include "PacketBase.h"
class NetworkSettings :
    public PacketBase
{
public:
	unsigned char ID() override;
	void Deserializ(std::vector<unsigned char> pack) override;
	uint16_t CompressionThreshold;
	uint16_t CompressionAlgorithm;
	bool ClientThrottle;
	uint8_t ClientThrottleThreshold;
	float ClientThrottleScalar;
};

