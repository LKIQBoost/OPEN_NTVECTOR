#pragma once
#include "PacketBase.h"
class ResourcePackClientResponse :
    public PacketBase
{
public:
	unsigned char ID() override;
	std::vector<unsigned char> Serializ() override;
	char Response;
	vector<string> PacksToDownload;
};

