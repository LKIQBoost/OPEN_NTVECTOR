#pragma once
#include "PacketBase.h"
class PyRpc :
    public PacketBase
{
public:
	unsigned char ID() override;
	void Deserializ(std::vector<unsigned char> pack) override;
	std::vector<unsigned char> Serializ() override;
	bool unknow = true;
	std::string rpcdata;
	unsigned int RpcHeader;
	bool unknow2;
};

