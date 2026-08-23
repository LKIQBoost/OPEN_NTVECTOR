#include "Disconnect.h"
#include "Logger.h"
#include <sstream>
#include <iomanip>

unsigned char Disconnect::ID()
{
    return IDDisconnect;
}

void Disconnect::Deserializ(std::vector<unsigned char> pack)
{
    LOG(LOG_WARN, "[Disconnect] Deserializing disconnect packet, size: ", pack.size());
    // 输出原始字节以便排查空消息问题
    {
        std::ostringstream hex_oss;
        hex_oss << "[Disconnect] Raw hex:";
        for (size_t i = 0; i < pack.size(); i++) {
            hex_oss << " " << std::hex << std::setfill('0') << std::setw(2) << (int)pack[i];
        }
        LOG(LOG_WARN, hex_oss.str());
    }
    BinaryReader br(pack.data(), pack.size());
    br.ReadInt16();
    // 新协议在 Int16 后增加了一个字节的 hideDisconnectScreen bool
    uint8_t hide = br.ReadUInt8();
    size_t length = 0;
    if (!hide) {
        length = br.ReadVarUInt();
    }
    message = string((char*)br.Read(length),length);
    LOG(LOG_WARN, "[Disconnect] Disconnect message: ", message);
    LOG(LOG_WARN, "[Disconnect] Message length: ", length);
    LOG(LOG_WARN, "[Disconnect] hideDisconnectScreen: ", (int)hide);
}
