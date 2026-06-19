#pragma once
#include "PacketBase.h"
class SettingsCommand :
	public PacketBase
{
public:
	unsigned char ID() override;
	std::vector<unsigned char> Serializ() override;
	std::string command;
	bool SuppressOutput = false;
};

