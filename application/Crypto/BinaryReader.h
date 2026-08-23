#include "BinaryWriter.h"
#include "EndianUtils.h"
#include <algorithm> // for std::move
#include <cstring>   // for memcpy
#pragma once
class BinaryReader
{
public:
    // 构造函数
    explicit BinaryReader(void* data, int length);

    // 析构函数
    ~BinaryReader();

    // 禁止拷贝构造和拷贝赋值
    BinaryReader(const BinaryReader&) = delete;
    BinaryReader& operator=(const BinaryReader&) = delete;

    // 移动构造函数
    BinaryReader(BinaryReader&& other) noexcept;

    // 移动赋值运算符
    BinaryReader& operator=(BinaryReader&& other) noexcept;

    // 写入数据到buffer
    void* Read(size_t size);
    int ReadInt32();
    unsigned int ReadUInt32();
    long long ReadInt64();
    unsigned long long ReadUInt64();
    short ReadInt16();
    unsigned short ReadUInt16();
    char ReadInt8();
    unsigned char ReadUInt8();
    int ReadVarInt_out(uint32_t&);
    int ReadVarInt();
    unsigned int ReadVarUInt();
    int64_t ReadVarInt64();

    long long ReadInt64_Big();
    unsigned long long ReadUInt64_Big();
    int ReadInt32_Big();
    unsigned int ReadUInt32_Big();
    short ReadInt16_Big();
    unsigned short ReadUInt16_Big();
    Vec3 ReadVec3();
    Vec2 ReadVec2();
    Vec3d ReadVec3d();
    // 在原有函数后面添加
    float ReadFloat();
    double ReadDouble();

    // 物品相关读取函数
    void ReadItemStack(ItemStack& item);
    void ReadItemInstance(ItemInstance& item);
    void ReadNBTData(std::unordered_map<std::string, std::string>& nbt);
    void ReadStringVector(std::vector<std::string>& strings);

    // 辅助读取函数
    uint32_t ReadVarInt32(int32_t& value);
    uint32_t ReadVarUInt32(uint32_t& value);
    void ReadBool(bool& value);
    void ReadByteSlice(std::vector<uint8_t>& data);
    void ReadStringUTF(std::string& str);
    std::string ReadString();




    // 获取buffer中的数据指针
    const uint8_t* data() const;

    size_t m_pointer;    // 当前缓冲区容量
private:
    void* m_reade(size_t size);
    uint8_t* m_buffer;    // 数据缓冲区
    size_t m_size;        // 当前数据大小

    // 内部扩容函数
    //void grow(size_t minCapacity);
};

