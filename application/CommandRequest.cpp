#include "CommandRequest.h"

unsigned char CommandRequest::ID()
{
    return IDCommandRequest;
}

std::vector<unsigned char> CommandRequest::Serializ()
{
    BinaryWriter bw(command.size() + RandomUUID.size() + 10);
    bw.WriteUInt8(ID());
    bw.WriteStringUTF(command);
    bw.WriteBool(false);
    bw.Write(RandomUUID.data(), RandomUUID.size());
    bw.WriteUInt32(5111808);
    return bw.vect();
}
