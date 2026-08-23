#include "NeteaseJson.h"

unsigned char NeteaseJson::ID()
{
    return IDNeteaseJson;
}

std::vector<unsigned char> NeteaseJson::Serializ()
{
	size_t size = Json.size();
	BinaryWriter Write(size+5);
	Write.WriteUInt8(ID());
	Write.WriteUInt8(Unknow);
	Write.WriteVarUInt(Json.size());
	Write.Write((char*)Json.data(),size);
	return Write.vect();
}
