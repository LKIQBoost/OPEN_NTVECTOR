#include "PlayStatus.h"

unsigned char PlayStatus::ID()
{
    return IDPlayStatus;
}

void PlayStatus::Deserializ(std::vector<unsigned char> pack)
{
    BinaryReader br(pack.data(), pack.size());
    Status = br.ReadUInt32_Big();
}
