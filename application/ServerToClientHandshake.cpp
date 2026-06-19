#include "ServerToClientHandshake.h"

unsigned char ServerToClientHandshake::ID()
{
    return IDServerToClientHandshake;
}

void ServerToClientHandshake::Deserializ(std::vector<unsigned char> pack)
{
    BinaryReader br(pack.data(), pack.size());
    uint32_t l = br.ReadVarInt();
    JWT = string((char*)br.Read(l),l);
}
