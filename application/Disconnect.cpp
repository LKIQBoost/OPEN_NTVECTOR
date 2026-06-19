#include "Disconnect.h"
#include "Logger.h"

unsigned char Disconnect::ID()
{
    return IDDisconnect;
}

void Disconnect::Deserializ(std::vector<unsigned char> pack)
{
    LOG(LOG_WARN, "[Disconnect] Deserializing disconnect packet, size: ", pack.size());
    BinaryReader br(pack.data(), pack.size());
    br.ReadInt16();
    size_t length = br.ReadVarUInt();
    message = string((char*)br.Read(length),length);
    LOG(LOG_WARN, "[Disconnect] Disconnect message: ", message);
}
