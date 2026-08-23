#include "CommandRequest.h"

extern unsigned int MinecraftBedrockProtocolVersion;

namespace {
constexpr unsigned int ProtocolV11960 = 567;
constexpr size_t CommandUuidSize = 16;

bool needsPlayerId(CommandOriginType originType)
{
    return originType == CommandOriginType::DevConsole || originType == CommandOriginType::Test;
}

void writeUuid(BinaryWriter& bw, const std::string& uuid)
{
    const size_t bytesToWrite = uuid.size() < CommandUuidSize ? uuid.size() : CommandUuidSize;
    bw.Write(uuid.data(), bytesToWrite);

    if (bytesToWrite < CommandUuidSize) {
        const unsigned char zeros[CommandUuidSize] = {};
        bw.Write(zeros, CommandUuidSize - bytesToWrite);
    }
}
}

unsigned char CommandRequest::ID()
{
    return IDCommandRequest;
}

std::vector<unsigned char> CommandRequest::Serializ()
{
    BinaryWriter bw(m_command.size() + m_requestID.size() + m_unknownString.size() + 64);
    bw.WriteUInt8(ID());
    bw.WriteStringUTF(m_command);
    bw.WriteVarInt(static_cast<char>(m_commandOrigin));
    writeUuid(bw, m_randomUUID);
    bw.WriteStringUTF(m_requestID);

    if (needsPlayerId(m_commandOrigin)) {
        bw.WriteVarInt64(m_playerID);
    }

    bw.WriteBool(m_internalSource);

    if (MinecraftBedrockProtocolVersion >= ProtocolV11960) {
        bw.WriteVarInt(m_commandVersion);
    }

    bw.WriteStringUTF(m_unknownString);

    return bw.vect();
}
