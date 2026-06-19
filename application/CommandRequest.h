#pragma once
#include "PacketBase.h"
class CommandRequest :
    public PacketBase
{
public:
	unsigned char ID() override;
	std::vector<unsigned char> Serializ() override;
    std::string RandomUUID;
    std::string command;
};

