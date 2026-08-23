#include "ClientToServerHandshake.h"

unsigned char ClientToServerHandshake::ID()
{
    return IDClientToServerHandshake;
}

std::vector<unsigned char> ClientToServerHandshake::Serializ()
{
    BinaryWriter bw(1);
    bw.WriteInt8(ID());
    return bw.vect();
}
