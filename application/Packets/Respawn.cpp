#include "Respawn.h"

// 辅助函数：读取 varuint64 (无符号变长 64 位整数)
static uint64_t ReadVarUInt64(BinaryReader& br) {
    uint64_t value;
    size_t len = BinaryWriter::varint_to_uint(
        reinterpret_cast<const uint8_t*>(br.data() + br.m_pointer),
        &value
    );
    br.m_pointer += len;
    return value;
}

unsigned char Respawn::ID()
{
    return IDRespawn;
}

void Respawn::Deserializ(std::vector<unsigned char> pack)
{
    BinaryReader br(pack.data(), pack.size());

    // 1. 读取 Position (Vec3: 3 个 float, 小端)
    Position = br.ReadVec3();

    // 2. 读取 State (uint8)
    State = br.ReadUInt8();

    // 3. 读取 EntityRuntimeID (varuint64)
    EntityRuntimeID = ReadVarUInt64(br);
}

std::vector<unsigned char> Respawn::Serializ()
{
    BinaryWriter bw(32);

    bw.WriteVarUInt32(ID());
    // 1. 写入 Position
    bw.WriteVec3(Position);

    // 2. 写入 State
    bw.WriteUInt8(State);

    // 3. 写入 EntityRuntimeID (varuint64)
    uint8_t varIntBuf[10];
    size_t len = BinaryWriter::uint_to_varint(EntityRuntimeID, varIntBuf);
    bw.Write(varIntBuf, len);

    return bw.vect();
}

const char* Respawn::GetStateString() const
{
    switch (State) {
    case RespawnStateSearchingForSpawn:
        return "SearchingForSpawn";
    case RespawnStateReadyToSpawn:
        return "ReadyToSpawn";
    case RespawnStateClientReadyToSpawn:
        return "ClientReadyToSpawn";
    default:
        return "Unknown";
    }
}
