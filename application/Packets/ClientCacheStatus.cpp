#include "ClientCacheStatus.h"

unsigned char ClientCacheStatus::ID()
{
    return IDClientCacheStatus;
}

std::vector<unsigned char> ClientCacheStatus::Serializ()
{
	BinaryWriter Write(3);
	Write.WriteUInt8(ID());
	Write.WriteUInt8(Enabled);
	Write.WriteUInt8(Unknow);
	return Write.vect();
}
