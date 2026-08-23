#include "ResourcePackClientResponse.h"

unsigned char ResourcePackClientResponse::ID()
{
    return IDResourcePackClientResponse;
}

std::vector<unsigned char> ResourcePackClientResponse::Serializ()
{
	BinaryWriter Write(4);
	Write.WriteUInt8(ID());
	Write.WriteUInt8(Response);
	Write.WriteUInt16(PacksToDownload.size());
	for (int i = 0;i < PacksToDownload.size();i++) {
		Write.Write(PacksToDownload[i].data(), PacksToDownload[i].size());
	}
	return Write.vect();
}
