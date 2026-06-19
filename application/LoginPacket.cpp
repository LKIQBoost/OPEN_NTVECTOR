#include "LoginPacket.h"
#include "Logger.h"

unsigned char LoginPacket::ID()
{
    return IDLogin;
}

std::vector<unsigned char> LoginPacket::Serializ()
{
	LOG(LOG_SERVER, "[LoginPacket] Serializing login packet");
	int chainsize = chain.size();
	int skinsize = skindata.size();
	LOG(LOG_SERVER, "[LoginPacket] Chain size: ", chainsize, ", Skin data size: ", skinsize);
	BinaryWriter bw(chainsize + skinsize + 8);
	bw.WriteUInt32(chainsize);
	bw.Write((const char*)chain.data(), chain.size());
	bw.WriteUInt32(skinsize);
	bw.Write((const char*)skindata.data(), skindata.size());
	int bwsize = bw.size();
	BinaryWriter Write(calculateVarintSizeFast(bwsize) + bwsize + 5);
	Write.WriteUInt8(ID());
	Write.WriteUInt32_Big(ProtocolVersion);
	Write.WriteVarUInt(bwsize);
	Write.Write(bw.data(),bwsize);
	LOG(LOG_SERVER, "[LoginPacket] Serialized, total size: ", Write.size(), ", protocol version: ", ProtocolVersion);
	return Write.vect();
}
