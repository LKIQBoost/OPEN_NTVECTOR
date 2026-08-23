#include "RequestChunkRadius.h"

unsigned char RequestChunkRadius::ID()
{
    return IDRequestChunkRadius;
}

std::vector<unsigned char> RequestChunkRadius::Serializ()
{
	BinaryWriter Write(3);
	Write.WriteUInt8(ID());
	Write.WriteVarUInt(ChunkRadius);
	Write.WriteUInt8(MaxChunkRadius);
	return Write.vect();
}
