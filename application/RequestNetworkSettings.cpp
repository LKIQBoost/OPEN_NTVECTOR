#include "RequestNetworkSettings.h"

unsigned char RequestNetworkSettings::ID() {
	return IDRequestNetworkSettings;
}
std::vector<unsigned char> RequestNetworkSettings::Serializ() {
	BinaryWriter Write(6);
	Write.WriteUInt8(ID());
	Write.WriteUInt8(1);
	Write.WriteUInt32_Big(ProtocolVersion);
	return Write.vect();
}