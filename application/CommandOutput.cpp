#include "CommandOutput.h"
#include <cstring>  // for memcpy

unsigned char CommandOutput::ID()
{
    return IDCommandOutput;
}

void CommandOutput::Deserializ(std::vector<unsigned char> pack)
{
    BinaryReader br(pack.data(), pack.size());

    // 1. 读取 CommandOrigin
    // Origin (varuint32)
    Origin.Origin = br.ReadVarUInt();

    // UUID (16 bytes, 固定长度)
    void* uuidData = br.Read(16);
    if (uuidData) {
        memcpy(Origin.UUID.data(), uuidData, 16);
    }

    // RequestID (string)
    br.ReadStringUTF(Origin.RequestID);

    // PlayerUniqueID (varint64) - 仅当 Origin == DevConsole 或 Test 时读取
    if (Origin.Origin == CommandOriginDevConsole || Origin.Origin == CommandOriginTest) {
        Origin.PlayerUniqueID = br.ReadVarInt64();
    }

    // 2. 读取 OutputType (uint8)
    OutputType = br.ReadUInt8();

    // 3. 读取 SuccessCount (varuint32)
    SuccessCount = br.ReadVarUInt();

    // 4. 读取 OutputMessages 数组
    uint32_t messageCount = br.ReadVarUInt();
    OutputMessages.clear();
    OutputMessages.reserve(messageCount);

    for (uint32_t i = 0; i < messageCount; i++) {
        CommandOutputMessage msg;

        // Success (bool)
        br.ReadBool(msg.Success);

        // Message (string)
        br.ReadStringUTF(msg.Message);

        // Parameters (string array)
        uint32_t paramCount = br.ReadVarUInt();
        msg.Parameters.reserve(paramCount);
        for (uint32_t j = 0; j < paramCount; j++) {
            std::string param;
            br.ReadStringUTF(param);
            msg.Parameters.push_back(std::move(param));
        }

        OutputMessages.push_back(std::move(msg));
    }

    // 5. 读取 DataSet (仅当 OutputType == CommandOutputTypeDataSet 时)
    if (OutputType == CommandOutputTypeDataSet) {
        br.ReadStringUTF(DataSet);
    }
}

std::vector<unsigned char> CommandOutput::Serializ()
{
    // CommandOutput 通常是服务端发送的，客户端一般不需要序列化
    // 但为了完整性，这里提供序列化实现
    BinaryWriter bw(256);

    // 1. 写入 CommandOrigin
    bw.WriteVarUInt(Origin.Origin);
    bw.Write(Origin.UUID.data(), 16);
    bw.WriteStringUTF(Origin.RequestID);
    if (Origin.Origin == CommandOriginDevConsole || Origin.Origin == CommandOriginTest) {
        // 写入 varint64 (zigzag 编码)
        uint8_t varIntBuf[10];
        // zigzag encode: (n << 1) ^ (n >> 63)
        uint64_t zigzag = (static_cast<uint64_t>(Origin.PlayerUniqueID) << 1) ^
                          static_cast<uint64_t>(Origin.PlayerUniqueID >> 63);
        size_t len = BinaryWriter::uint_to_varint(zigzag, varIntBuf);
        bw.Write(varIntBuf, len);
    }

    // 2. 写入 OutputType
    bw.WriteUInt8(OutputType);

    // 3. 写入 SuccessCount
    bw.WriteVarUInt(SuccessCount);

    // 4. 写入 OutputMessages
    bw.WriteVarUInt(static_cast<uint32_t>(OutputMessages.size()));
    for (const auto& msg : OutputMessages) {
        bw.WriteBool(msg.Success);
        bw.WriteStringUTF(msg.Message);
        bw.WriteVarUInt(static_cast<uint32_t>(msg.Parameters.size()));
        for (const auto& param : msg.Parameters) {
            bw.WriteStringUTF(param);
        }
    }

    // 5. 写入 DataSet
    if (OutputType == CommandOutputTypeDataSet) {
        bw.WriteStringUTF(DataSet);
    }

    return bw.vect();
}

std::vector<std::string> CommandOutput::GetAllMessages() const
{
    std::vector<std::string> result;
    result.reserve(OutputMessages.size());
    for (const auto& msg : OutputMessages) {
        result.push_back(msg.Message);
    }
    return result;
}
