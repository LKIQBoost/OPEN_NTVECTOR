#include "CommandBlockUpdate.h"

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

// 辅助函数：读取 BlockPos (UBlockPos - 使用 varint 编码)
static BlockPos ReadUBlockPos(BinaryReader& br) {
    BlockPos pos;
    // UBlockPos 使用有符号 varint 编码
    pos.x = br.ReadVarInt();
    pos.y = static_cast<int32_t>(br.ReadVarUInt());  // y 通常是无符号的
    pos.z = br.ReadVarInt();
    return pos;
}

// 辅助函数：写入 BlockPos (UBlockPos - 使用 varint 编码)
static void WriteUBlockPos(BinaryWriter& bw, const BlockPos& pos) {
    bw.WriteVarInt(pos.x);
    bw.WriteVarUInt(static_cast<uint32_t>(pos.y));  // y 使用无符号
    bw.WriteVarInt(pos.z);
}

unsigned char CommandBlockUpdate::ID()
{
    return IDCommandBlockUpdate;
}

void CommandBlockUpdate::Deserializ(std::vector<unsigned char> pack)
{
    BinaryReader br(pack.data(), pack.size());
    // 1. 读取 Block 标志
    br.ReadBool(Block);

    if (Block) {
        // 物理命令方块
        // 2. 读取 Position (UBlockPos)
        Position = ReadUBlockPos(br);

        // 3. 读取 Mode (varuint32)
        br.ReadVarUInt32(Mode);

        // 4. 读取 NeedsRedstone (bool)
        br.ReadBool(NeedsRedstone);

        // 5. 读取 Conditional (bool)
        br.ReadBool(Conditional);
    }
    else {
        // 矿车中的命令方块
        // 2. 读取 MinecartEntityRuntimeID (varuint64)
        MinecartEntityRuntimeID = ReadVarUInt64(br);
    }

    // 6. 读取 Command (string)
    Command = br.ReadString();

    // 7. 读取 LastOutput (string)
    LastOutput = br.ReadString();

    // 8. 读取 Name (string)
    Name = br.ReadString();

    // 8.5. 读取 FilteredName (string) - 1.21.120 协议字段
    FilteredName = br.ReadString();

    // 9. 读取 ShouldTrackOutput (bool)
    br.ReadBool(ShouldTrackOutput);

    // 10. 读取 TickDelay (uint32, 网易版)
    TickDelay = br.ReadUInt32();

    // 11. 读取 ExecuteOnFirstTick (bool)
    br.ReadBool(ExecuteOnFirstTick);
}

std::vector<unsigned char> CommandBlockUpdate::Serializ()
{
    BinaryWriter bw(256);

    // 写入数据包ID - 必须用 VarUInt 编码，不是 UInt8
    bw.WriteVarUInt(ID());
    // 1. 写入 Block 标志
    bw.WriteBool(Block);

    if (Block) {
        // 物理命令方块
        // 2. 写入 Position (UBlockPos)
        WriteUBlockPos(bw, Position);

        // 3. 写入 Mode (varuint32)
        bw.WriteVarUInt32(Mode);

        // 4. 写入 NeedsRedstone (bool)
        bw.WriteBool(NeedsRedstone);

        // 5. 写入 Conditional (bool)
        bw.WriteBool(Conditional);
    }
    else {
        // 矿车中的命令方块
        // 2. 写入 MinecartEntityRuntimeID (varuint64)
        uint8_t varIntBuf[10];
        size_t len = BinaryWriter::uint_to_varint(MinecartEntityRuntimeID, varIntBuf);
        bw.Write(varIntBuf, len);
    }

    // 6. 写入 Command (string)
    bw.WriteStringUTF(Command);

    // 7. 写入 LastOutput (string)
    bw.WriteStringUTF(LastOutput);

    // 8. 写入 Name (string)
    bw.WriteStringUTF(Name);

    // 8.5. 写入 FilteredName (string) - 1.21.120 协议必需字段，缺失会导致服务端解析错位被踢
    bw.WriteStringUTF(FilteredName);

    // 9. 写入 ShouldTrackOutput (bool)
    bw.WriteBool(ShouldTrackOutput);

    // 10. 写入 TickDelay (uint32, 网易版)
    bw.WriteUInt32(TickDelay);

    // 11. 写入 ExecuteOnFirstTick (bool)
    bw.WriteBool(ExecuteOnFirstTick);

    return bw.vect();
}

const char* CommandBlockUpdate::GetModeString() const
{
    switch (Mode) {
    case CommandBlockModeImpulse:
        return "Impulse";
    case CommandBlockModeRepeating:
        return "Repeating";
    case CommandBlockModeChain:
        return "Chain";
    default:
        return "Unknown";
    }
}
