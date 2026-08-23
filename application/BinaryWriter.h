#pragma once
#ifndef DYNAMIC_BUFFER_H
#define DYNAMIC_BUFFER_H

#include "EndianUtils.h"
#include "platform/PlatformCompat.h"
#include <vector>
#include <cstddef> // for size_t
#include <cstdint> // for uint8_t
#include <algorithm> // for std::reverse
#include <unordered_map>
#include <string>

struct Vec3 {
    float x;
    float y;
    float z;
};
struct Vec2 {
    float x;
    float y;
};
struct Vec3d {
    int x;
    int y;
    int z;
};

// ��Ʒ��ջ�ṹ
struct ItemStack {
    int32_t NetworkID = 0;           // ��Ʒ����ID
    uint16_t Count = 0;              // ����
    uint32_t MetadataValue = 0;      // ����ֵ/�;�ֵ
    int32_t BlockRuntimeID = 0;      // ��������ʱID

    // NBT���ݣ�ʹ���ַ�����Variant��ӳ�䣩
    std::unordered_map<std::string, std::string> NBTData;

    // ��������
    std::vector<std::string> CanBePlacedOn;  // ���Է����ڸ÷�����
    std::vector<std::string> CanBreak;       // �����ƻ��ķ���

    // Ĭ�Ϲ��캯��
    ItemStack() = default;

    // ����Ƿ��ǿ�����Ʒ
    bool IsAir() const {
        return NetworkID == 0 || NetworkID == -1;
    }
};

// ��Ʒʵ���ṹ
struct ItemInstance {
    ItemStack Stack;
    int32_t StackNetworkID = 0;      // ��ջ����ID������ͬ����
    bool HasNetworkID = false;       // �Ƿ�������ID

    // Ĭ�Ϲ��캯��
    ItemInstance() = default;

    // ����Ƿ��ǿ�����Ʒ
    bool IsAir() const {
        return Stack.IsAir();
    }
};




// ������Ľṹ�嶨��
struct EntityMetadataBasic {
    // ��ֵ����Ŀ
    struct Entry {
        uint32_t key;
        uint32_t type;
        std::vector<uint8_t> value;  // ԭʼ�ֽ�����
    };

    // ������Ŀ
    std::vector<Entry> entries;
};

class BinaryWriter {
public:
    // ���캯����Ĭ�ϳ�ʼ����Ϊ256�ֽ�
    explicit BinaryWriter(size_t initialCapacity = 256);

    // ��������
    ~BinaryWriter();

    // ��ֹ��������Ϳ�����ֵ
    BinaryWriter(const BinaryWriter&) = delete;
    BinaryWriter& operator=(const BinaryWriter&) = delete;

    // �ƶ����캯��
    BinaryWriter(BinaryWriter&& other) noexcept;

    // �ƶ���ֵ�����
    BinaryWriter& operator=(BinaryWriter&& other) noexcept;

    static void WriteVarint(std::vector<unsigned char> &data);

    // д�����ݵ�buffer
    void Write(const void* data, size_t size);
    void WriteInt32(int data);
    void WriteUInt32(unsigned int data);
    void WriteInt64(long long data);
    void WriteUInt64(unsigned long long data);
    void WriteInt16(short data);
    void WriteUInt16(unsigned short data);
    void WriteInt8(char data);
    void WriteUInt8(unsigned char data);
    void WriteVarInt(int data);
    void WriteVarUInt(unsigned int data);
    void WriteVarInt64(__int64 data);
    void WriteVarUInt64(unsigned __int64 data);

    void WriteInt32_Big(int data);
    void WriteUInt32_Big(unsigned int data);
    void WriteInt64_Big(long long data);
    void WriteUInt64_Big(unsigned long long data);
    void WriteInt16_Big(short data);
    void WriteUInt16_Big(unsigned short data);
    void WriteVec3(const Vec3& data);
    void WriteVec2(const Vec2& data);
    void WriteVec3d(const Vec3d& data);
    static size_t int_to_varint(int64_t value, uint8_t* out);
    static size_t uint_to_varint(uint64_t value, uint8_t* out);
    static size_t varint_to_int(const uint8_t* data, int64_t* value);
    static size_t varint_to_uint(const uint8_t* data, uint64_t* value);
    static size_t calculateVarintSizeFast(int32_t value);

    // ��ȡbuffer�е�����ָ��
    const uint8_t* data() const;

    std::vector<unsigned char> vect();
    // ��ȡ��ǰ���ݴ�С
    size_t size() const;

    // ��ȡ��ǰbuffer����
    size_t capacity() const;

    // ���buffer(���ͷ��ڴ�)
    void clear();

    // Ԥ���ռ�
    void reserve(size_t newCapacity);

    // ����buffer��ʵ�ʴ�С
    void shrinkToFit();
    void WriteFloat(float data);
    void WriteDouble(double data);
    // ��Ʒ���д�뺯��
    void WriteItemStack(const ItemStack& item);
    void WriteItemInstance(const ItemInstance& item);
    void WriteNBTData(const std::unordered_map<std::string, std::string>& nbt);
    void WriteStringVector(const std::vector<std::string>& strings);


    // д��Varint32�����еı䳤����д�룩
    void WriteVarInt32(int32_t value);
    void WriteVarUInt32(uint32_t value);
    void WriteBool(bool value);
    void WriteByteSlice(const uint8_t* data, size_t size);
    void WriteStringUTF(const std::string& str);
private:
    uint8_t* m_buffer;    // ���ݻ�����
    size_t m_size;        // ��ǰ���ݴ�С
    size_t m_capacity;    // ��ǰ����������

    // �ڲ����ݺ���
    void grow(size_t minCapacity);
};

#endif // DYNAMIC_BUFFER_H