#pragma once
#include "PacketBase.h"
class Text :
    public PacketBase
{
public:
	uint8_t ID() override;
	void Deserializ(std::vector<unsigned char> pack) override;
	std::vector<unsigned char> Serializ() override;
	uint8_t type;
	std::string _data;
	std::string _data2;
	std::string SysMsg;
	std::string PlayerID;
};

