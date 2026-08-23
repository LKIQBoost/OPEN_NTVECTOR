#include "Packets/CommandRequest.h"
#include "Crypto/BinaryReader.h"

#include <cassert>
#include <cstring>
#include <iostream>

unsigned int MinecraftBedrockProtocolVersion = 567;

int main()
{
    CommandRequest packet;
    packet.SetCommand("say hello");
    packet.SetCommandOrigin(CommandOriginType::Player);
    packet.SetRandomUUID("0123456789abcdef");
    packet.SetRequestID("request-1");
    packet.SetInternalSource(true);
    packet.SetCommandVersion(5111808);
    packet.SetUnknownString("tail");

    assert(packet.GetCommand() == "say hello");
    assert(packet.GetCommandOrigin() == CommandOriginType::Player);
    assert(packet.GetRandomUUID().size() == 16);

    const std::vector<unsigned char> bytes = packet.Serializ();
    BinaryReader reader(const_cast<unsigned char*>(bytes.data()), static_cast<int>(bytes.size()));
    assert(reader.ReadUInt8() == IDCommandRequest);

    std::string command;
    reader.ReadStringUTF(command);
    assert(command == "say hello");
    assert(reader.ReadVarInt() == static_cast<int>(CommandOriginType::Player));

    const char* uuid = static_cast<const char*>(reader.Read(16));
    assert(std::memcmp(uuid, "0123456789abcdef", 16) == 0);

    std::string requestId;
    reader.ReadStringUTF(requestId);
    assert(requestId == "request-1");
    bool internalSource = false;
    reader.ReadBool(internalSource);
    assert(internalSource);
    const uint32_t rawCommandVersion = reader.ReadVarUInt();
    const int commandVersion = static_cast<int>(rawCommandVersion >> 1) ^
        -static_cast<int>(rawCommandVersion & 1);
    assert(commandVersion == 5111808);
    std::string tail;
    reader.ReadStringUTF(tail);
    assert(tail == "tail");
    assert(reader.m_pointer == bytes.size());

    std::cout << "CommandRequest interface test passed\n";
    return 0;
}
