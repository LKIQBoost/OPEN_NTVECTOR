#include "PacketBase.h"

unsigned char PacketBase::ID() {
	return 0;
}
std::vector<unsigned char> PacketBase::Serializ() {
	return std::vector<unsigned char>();
}
void PacketBase::Deserializ(std::vector<unsigned char> pack) {
	return;
}