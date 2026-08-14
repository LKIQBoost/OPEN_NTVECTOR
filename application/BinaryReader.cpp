#include "BinaryReader.h"
BinaryReader::BinaryReader(void* data, int length)
    : m_buffer(nullptr), m_size(0), m_pointer(0) {
    if (length == 0)
        return;
    m_size = length;
    m_buffer = new uint8_t[length];
    memset(m_buffer,'\0',length);
    memcpy(m_buffer, data, length);
}

BinaryReader::~BinaryReader() {
    delete[] m_buffer;
}
BinaryReader::BinaryReader(BinaryReader&& other) noexcept
    : m_buffer(other.m_buffer),
    m_size(other.m_size),
    m_pointer(other.m_pointer) {
    other.m_buffer = nullptr;
    other.m_size = 0;
    other.m_pointer = 0;
}
BinaryReader& BinaryReader::operator=(BinaryReader&& other) noexcept {
    if (this != &other) {
        delete[] m_buffer;

        m_buffer = other.m_buffer;
        m_size = other.m_size;
        m_pointer = other.m_pointer;

        other.m_buffer = nullptr;
        other.m_size = 0;
        other.m_pointer = 0;
    }
    return *this;
}
void* BinaryReader::Read(size_t size) {
    if (size == 0) return 0;

    void* p = (void*)(m_buffer + m_pointer);
    m_pointer += size;
    return p;
}
void* BinaryReader::m_reade(size_t size) {
    if (size == 0) return 0;

    void* p = (void*)(m_buffer + m_pointer);
    //void* p = m_buffer;
    m_pointer += size;
    return p;
}
// 结构体读取
Vec3 BinaryReader::ReadVec3() {
    Vec3* data = (Vec3*)m_reade(sizeof(Vec3));
    return data ? *data : Vec3{ 0, 0, 0 };
}

Vec2 BinaryReader::ReadVec2() {
    Vec2* data = (Vec2*)m_reade(sizeof(Vec2));
    return data ? *data : Vec2{ 0, 0 };
}

Vec3d BinaryReader::ReadVec3d() {
    Vec3d* data = (Vec3d*)m_reade(sizeof(Vec3d));
    return data ? *data : Vec3d{ 0, 0, 0 };
}
int BinaryReader::ReadInt32() {
    int* data = (int*)m_reade(4);
    return *data;
}
unsigned int BinaryReader::ReadUInt32() {
    unsigned int* data = (unsigned int*)m_reade(4);
    return *data;
}
short BinaryReader::ReadInt16() {
    short* data = (short*)m_reade(2);
    return *data;
}
unsigned short BinaryReader::ReadUInt16() {
    unsigned short* data = (unsigned short*)m_reade(2);
    return *data;
}
char BinaryReader::ReadInt8() {
    char* data = (char*)m_reade(1);
    return *data;
}
unsigned char BinaryReader::ReadUInt8() {
    unsigned char* data = (unsigned char*)m_reade(1);
    return *data;
}



int BinaryReader::ReadInt32_Big() {
    int* data = (int*)m_reade(4);
    int num = *data;
    num = EndianUtils::SwapEndian<int>(num);
    return num;
}
unsigned int BinaryReader::ReadUInt32_Big() {
    unsigned int* data = (unsigned int*)m_reade(4);
    unsigned int num = *data;
    num = EndianUtils::SwapEndian<unsigned int>(num);
    return num;
}
short BinaryReader::ReadInt16_Big() {
    short* data = (short*)m_reade(2);
    short num = *data;
    num = EndianUtils::SwapEndian<short>(num);
    return num;
}
unsigned short BinaryReader::ReadUInt16_Big() {
    unsigned short* data = (unsigned short*)m_reade(2);
    unsigned short num = *data;
    num = EndianUtils::SwapEndian<unsigned short>(num);
    return num;
}

const uint8_t* BinaryReader::data() const {
    return m_buffer;
}

int BinaryReader::ReadVarInt() {
    int64_t length;
    size_t len = BinaryWriter::varint_to_int(m_buffer + m_pointer, &length);
    m_pointer += len;
    return length;
}
int BinaryReader::ReadVarInt_out(uint32_t& readpot) {
    int64_t length;
    size_t len = BinaryWriter::varint_to_int(m_buffer + m_pointer, &length);
    m_pointer += len;
    readpot = len;
    return length;
}
unsigned int BinaryReader::ReadVarUInt() {
    uint64_t length;
    size_t len = BinaryWriter::varint_to_uint(m_buffer + m_pointer, &length);
    m_pointer += len;
    return length;
}

int64_t BinaryReader::ReadVarInt64()
{
    int64_t length;
    size_t len = BinaryWriter::varint_to_int(m_buffer + m_pointer, &length);
    m_pointer += len;
    return length;
}

long long BinaryReader::ReadInt64_Big()
{
    long long* data = (long long*)m_reade(8);
    long long num = *data;
    num = EndianUtils::SwapEndian<long long>(num);
    return num;
}

unsigned long long BinaryReader::ReadUInt64_Big()
{
    unsigned long long* data = (unsigned long long*)m_reade(8);
    unsigned long long num = *data;
    num = EndianUtils::SwapEndian<unsigned long long>(num);
    return num;
}

long long BinaryReader::ReadInt64()
{
    long long* data = (long long*)m_reade(8);
    return *data;
}
unsigned long long BinaryReader::ReadUInt64()
{
    unsigned long long* data = (unsigned long long*)m_reade(8);
    return *data;
}
float BinaryReader::ReadFloat() {
    float* data = (float*)m_reade(sizeof(float));
    return *data;
}

double BinaryReader::ReadDouble() {
    double* data = (double*)m_reade(sizeof(double));
    return *data;
}

// 读取字符串向量
void BinaryReader::ReadStringVector(std::vector<std::string>& strings) {
    int32_t count = 0;
    ReadVarInt32(count);

    strings.clear();
    strings.reserve(count);

    for (int32_t i = 0; i < count; i++) {
        std::string str;
        ReadStringUTF(str);
        strings.push_back(std::move(str));
    }
}

// 读取NBT数据（简化版本）
void BinaryReader::ReadNBTData(std::unordered_map<std::string, std::string>& nbt) {
    nbt.clear();

    int16_t length = ReadInt16();
    if (length <= 0) {
        return;
    }

    // 简单实现：读取文本格式
    std::vector<uint8_t> nbtBytes(length);
    memcpy(nbtBytes.data(), Read(length), length);

    std::string nbtText(reinterpret_cast<char*>(nbtBytes.data()), length);

    // 解析键值对（简单实现）
    size_t pos = 0;
    while (pos < nbtText.size()) {
        size_t colonPos = nbtText.find(':', pos);
        if (colonPos == std::string::npos) break;

        size_t semicolonPos = nbtText.find(';', colonPos);
        if (semicolonPos == std::string::npos) break;

        std::string key = nbtText.substr(pos, colonPos - pos);
        std::string value = nbtText.substr(colonPos + 1, semicolonPos - colonPos - 1);

        nbt[key] = value;
        pos = semicolonPos + 1;
    }
}

// 读取物品堆栈
void BinaryReader::ReadItemStack(ItemStack& item) {

    // 读取网络ID
    item.NetworkID = ReadVarInt();

    // PhoenixBuilder specific changes 特殊处理
    if (item.NetworkID == 0 || item.NetworkID == -1) {
        item.Count = 0;
        item.MetadataValue = 0;
        item.BlockRuntimeID = 0;
        item.NBTData.clear();
        item.CanBePlacedOn.clear();
        item.CanBreak.clear();
        return;
    }

    // 读取基础信息
    item.Count = ReadUInt16();
    item.MetadataValue = ReadVarUInt();

    // 读取是否有网络ID
    bool hasNetID = false;
    ReadBool(hasNetID);

    // 如果有网络ID，读取之（这里暂时跳过，因为ItemStack没有这个字段）
    if (hasNetID) {
        int32_t stackNetworkID = ReadVarInt();
        // 这个字段属于ItemInstance，不是ItemStack
        // 所以这里只是跳过数据
    }

    // 读取方块运行时ID
    item.BlockRuntimeID = ReadVarInt();

    // 读取额外数据
    int32_t extraDataSize = ReadVarInt();
    if (extraDataSize > 0) {
        // 创建一个临时的BinaryReader来解析额外数据
        const uint8_t* extraData = reinterpret_cast<const uint8_t*>(Read(extraDataSize));
        BinaryReader extraReader((void*)extraData, extraDataSize);

        // 读取NBT
        int16_t length = extraReader.ReadInt16();
        if (length == -1) {
            // 有版本号
            uint8_t version = extraReader.ReadUInt8();
            if (version == 1) {
                extraReader.ReadNBTData(item.NBTData);
            }
        }
        else if (length > 0) {
            extraReader.ReadNBTData(item.NBTData);
        }

        // 读取交互属性
        extraReader.ReadStringVector(item.CanBePlacedOn);
        extraReader.ReadStringVector(item.CanBreak);

        // 如果是盾牌，读取格挡时间戳
        if (item.NetworkID == 513) {  // 假设513是盾牌的ID
            int64_t blockingTick = extraReader.ReadInt64();
            // 这里可以存储到NBTData中
            item.NBTData["BlockingTick"] = std::to_string(blockingTick);
        }
    }
}

// 读取物品实例
void BinaryReader::ReadItemInstance(ItemInstance& item) {
    // 读取堆栈信息
    ReadItemStack(item.Stack);

    // 如果物品是空气，直接返回
    if (item.Stack.IsAir()) {
        item.HasNetworkID = false;
        item.StackNetworkID = 0;
        return;
    }

    // 读取是否有网络ID
    item.HasNetworkID = ReadUInt8() != 0;

    if (item.HasNetworkID) {
        item.StackNetworkID = ReadVarInt();
    }
}

// 辅助函数实现
uint32_t BinaryReader::ReadVarInt32(int32_t& value) {
    uint32_t temp;
    value = static_cast<int32_t>(ReadVarInt_out(temp));
    return temp;
}

uint32_t BinaryReader::ReadVarUInt32(uint32_t& value) {
    uint32_t temp;
    value = static_cast<uint32_t>(ReadVarInt_out(temp));
    return temp;
}

void BinaryReader::ReadBool(bool& value) {
    value = ReadUInt8() != 0;
}

void BinaryReader::ReadByteSlice(std::vector<uint8_t>& data) {
    int32_t size = 0;
    ReadVarInt32(size);

    data.resize(size);
    if (size > 0) {
        memcpy(data.data(), Read(size), size);
    }
}

void BinaryReader::ReadStringUTF(std::string& str) {
    int32_t size = 0;
    ReadVarInt32(size);

    str.resize(size);
    if (size > 0) {
        memcpy(&str[0], Read(size), size);
    }
}
std::string BinaryReader::ReadString()
{
    size_t length = ReadVarUInt();
    return std::string((char*)Read(length), length);
}