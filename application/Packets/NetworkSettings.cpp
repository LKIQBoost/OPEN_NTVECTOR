#include "NetworkSettings.h"
#include "Logger.h"

unsigned char NetworkSettings::ID()
{
    return IDNetworkSettings;
}

void NetworkSettings::Deserializ(std::vector<unsigned char> pack)
{
    LOG(LOG_NETWORK, "[NetworkSettings] Deserializing network settings, size: ", pack.size());
    BinaryReader br(pack.data(),pack.size());
    CompressionThreshold = br.ReadUInt16();
    CompressionAlgorithm = br.ReadUInt16();
    ClientThrottle = br.ReadInt8();
    ClientThrottleThreshold = br.ReadInt8();
    ClientThrottleScalar = (float)br.ReadUInt32();
    LOG(LOG_NETWORK, "[NetworkSettings] CompressionThreshold: ", CompressionThreshold);
    LOG(LOG_NETWORK, "[NetworkSettings] CompressionAlgorithm: ", CompressionAlgorithm);
    LOG(LOG_NETWORK, "[NetworkSettings] ClientThrottle: ", (int)ClientThrottle);
    LOG(LOG_NETWORK, "[NetworkSettings] ClientThrottleThreshold: ", (int)ClientThrottleThreshold);
    LOG(LOG_NETWORK, "[NetworkSettings] ClientThrottleScalar: ", ClientThrottleScalar);
}
