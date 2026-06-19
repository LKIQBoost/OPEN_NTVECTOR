#pragma once
#include "PacketBase.h"
class RequestChunkRadius :
    public PacketBase
{
public:
	unsigned char ID() override;
	std::vector<unsigned char> Serializ() override;
	int32_t ChunkRadius;
		/*
			PhoenixBuilder specific changes.
			Changes Maker: Liliya233
			Committed by Happy2018new.

			MaxChunkRadius is the maximum chunk radius that the player wants to receive. The reason for the client sending this
			is currently unknown.

			For netease, the data type of this field is uint8,
			but on standard minecraft, this is int32.
		*/
	uint8_t MaxChunkRadius;
};

