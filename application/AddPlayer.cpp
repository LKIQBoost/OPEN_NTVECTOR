#include "AddPlayer.h"

void AddPlayer::Deserializ(std::vector<unsigned char> pack)
{
	BinaryReader br(pack.data(), pack.size());
	uuid = std::string((char*)br.Read(16), 16);
	userName = br.ReadString();
}
