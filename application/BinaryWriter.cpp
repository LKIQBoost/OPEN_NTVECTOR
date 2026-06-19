#include "BinaryWriter.h"

BinaryWriter::BinaryWriter(size_t initialCapacity)
    : m_buffer(nullptr), m_size(0), m_capacity(0) {
    if (initialCapacity > 0) {
        reserve(initialCapacity);
    }
}

BinaryWriter::~BinaryWriter() {
    delete[] m_buffer;
}

BinaryWriter::BinaryWriter(BinaryWriter&& other) noexcept
    : m_buffer(other.m_buffer),
    m_size(other.m_size),
    m_capacity(other.m_capacity) {
    other.m_buffer = nullptr;
    other.m_size = 0;
    other.m_capacity = 0;
}

BinaryWriter& BinaryWriter::operator=(BinaryWriter&& other) noexcept {
    if (this != &other) {
        delete[] m_buffer;

        m_buffer = other.m_buffer;
        m_size = other.m_size;
        m_capacity = other.m_capacity;

        other.m_buffer = nullptr;
        other.m_size = 0;
        other.m_capacity = 0;
    }
    return *this;
}

void BinaryWriter::WriteVarint(std::vector<unsigned char> &data)
{
    uint8_t varint[10];
    int leng = BinaryWriter::uint_to_varint(data.size(), varint);
    data.insert(data.begin(), varint, varint + leng);
}

void BinaryWriter::Write(const void* data, size_t size) {
    if (size == 0) return;

    // ����Ƿ���Ҫ����
    if (m_size + size > m_capacity) {
        grow(m_size + size);
    }

    // �������ݵ�������
    memcpy(m_buffer + m_size, data, size);
    m_size += size;
}

// �ṹ��д�뺯��
void BinaryWriter::WriteVec3(const Vec3& data) {
    Write(&data, sizeof(Vec3));  // ֱ��д�������ṹ��
}

void BinaryWriter::WriteVec2(const Vec2& data) {
    Write(&data, sizeof(Vec2));
}

void BinaryWriter::WriteVec3d(const Vec3d& data) {
    Write(&data, sizeof(Vec3d));
}
void BinaryWriter::WriteInt32(int data) {
    Write(&data, 4);
}
void BinaryWriter::WriteUInt32(unsigned int data){
    Write(&data, 4);
}
void BinaryWriter::WriteInt16(short data){
    Write(&data, 2);
}
void BinaryWriter::WriteUInt16(unsigned short data){
    Write(&data, 2);
}
void BinaryWriter::WriteInt8(char data){
    Write(&data, 1);
}
void BinaryWriter::WriteUInt8(unsigned char data){
    Write(&data, 1);
}
void BinaryWriter::WriteVarInt(int data) {
    uint8_t out[10];
    size_t len = int_to_varint(data, out);
    Write(out, len);
}
void BinaryWriter::WriteVarInt64(__int64 data) {
    uint8_t out[10];
    size_t len = int_to_varint(data, out);
    Write(out, len);
}

void BinaryWriter::WriteVarUInt(unsigned int data)
{
    uint8_t out[10];
    size_t len = uint_to_varint(data, out);
    Write(out, len);
}
void BinaryWriter::WriteVarUInt64(unsigned __int64 data)
{
    uint8_t out[10];
    size_t len = uint_to_varint(data, out);
    Write(out, len);
}

void BinaryWriter::WriteInt32_Big(int data) {
    int data_ = EndianUtils::SwapEndian<int>(data);
    Write(&data_,4);
}
void BinaryWriter::WriteUInt32_Big(unsigned int data) {
    unsigned int data_ = EndianUtils::SwapEndian<unsigned int>(data);
    Write(&data_, 4);
}
void BinaryWriter::WriteInt64_Big(long long data)
{
    int data_ = EndianUtils::SwapEndian<unsigned long long>(data);
    Write(&data_, 8);
}
void BinaryWriter::WriteUInt64_Big(unsigned long long data)
{
    unsigned int data_ = EndianUtils::SwapEndian<unsigned long long>(data);
    Write(&data_, 8);
}
void BinaryWriter::WriteInt64(long long data)
{
    long long data_ = data;
    Write(&data_, 8);
}
void BinaryWriter::WriteUInt64(unsigned long long data)
{
    unsigned long long data_ = data;
    Write(&data_, 8);
}
void BinaryWriter::WriteInt16_Big(short data) {
    short data_ = EndianUtils::SwapEndian<short>(data);
    Write(&data_, 2);
}
void BinaryWriter::WriteUInt16_Big(unsigned short data) {
    unsigned short data_ = EndianUtils::SwapEndian<unsigned short>(data);
    Write(&data_, 2);
}
size_t BinaryWriter::int_to_varint(int64_t value, uint8_t* out) {
    // ʹ��zigzag���봦���з�������
    uint64_t zigzag = (value << 1) ^ (value >> 63);
    size_t len = 0;

    do {
        uint8_t byte = zigzag & 0x7F;
        zigzag >>= 7;

        if (zigzag != 0) {
            byte |= 0x80;  // �������λ��ʾ���к����ֽ�
        }

        out[len++] = byte;
    } while (zigzag != 0);

    return len;
}
size_t BinaryWriter::uint_to_varint(uint64_t value, uint8_t* out) {
    size_t len = 0;

    do {
        uint8_t byte = value & 0x7F;
        value >>= 7;

        if (value != 0) {
            byte |= 0x80;
        }

        out[len++] = byte;
    } while (value != 0);

    return len;
}
size_t BinaryWriter::calculateVarintSizeFast(int32_t value) {
    uint32_t v = static_cast<uint32_t>(value < 0 ? ~value + 1 : value);
    if (v < (1 << 7)) return 1;
    if (v < (1 << 14)) return 2;
    if (v < (1 << 21)) return 3;
    if (v < (1 << 28)) return 4;
    return 5;  // int32_t ��� 5 �ֽڣ�������λ��
}
size_t BinaryWriter::varint_to_int(const uint8_t* data, int64_t* value) {
    int64_t result = 0;
    size_t len = 0;
    unsigned shift = 0;

    while (true) {
        if (shift >= 64 || len >= 10) {
            return 0; // ������������
        }

        uint8_t byte = data[len++];
        result |= (uint64_t)(byte & 0x7F) << shift;
        shift += 7;

        if ((byte & 0x80) == 0) {
            break;
        }
    }

    *value = result;
    return len;
}
size_t BinaryWriter::varint_to_uint(const uint8_t* data, uint64_t* value) {
    uint64_t result = 0;
    size_t len = 0;
    unsigned shift = 0;

    while (true) {
        if (shift >= 64 || len >= 10) {
            return 0; // ������������
        }

        uint8_t byte = data[len++];
        result |= (uint64_t)(byte & 0x7F) << shift;
        shift += 7;

        if ((byte & 0x80) == 0) {
            break;
        }
    }

    *value = result;
    return len;
}



const uint8_t* BinaryWriter::data() const {
    return m_buffer;
}

std::vector<unsigned char> BinaryWriter::vect()
{
    int size = this->size();
    std::vector<unsigned char> result(size);
    char* data = (char*)(this->data());
    result.assign(data, data + size);
    return result;
}

size_t BinaryWriter::size() const {
    return m_size;
}

size_t BinaryWriter::capacity() const {
    return m_capacity;
}

void BinaryWriter::clear() {
    m_size = 0;
}

void BinaryWriter::reserve(size_t newCapacity) {
    if (newCapacity <= m_capacity) return;

    grow(newCapacity);
}

void BinaryWriter::shrinkToFit() {
    if (m_size == m_capacity) return;

    if (m_size == 0) {
        delete[] m_buffer;
        m_buffer = nullptr;
        m_capacity = 0;
    }
    else {
        uint8_t* newBuffer = new uint8_t[m_size];
        memcpy(newBuffer, m_buffer, m_size);
        delete[] m_buffer;
        m_buffer = newBuffer;
        m_capacity = m_size;
    }
}

void BinaryWriter::grow(size_t minCapacity) {
    // �����µ�������ͨ���ǵ�ǰ������1.5����2��
    size_t newCapacity = m_capacity * 2;
    if (newCapacity < minCapacity) {
        newCapacity = minCapacity;
    }

    // �������ڴ�
    uint8_t* newBuffer = new uint8_t[newCapacity];

    // ����������
    if (m_buffer != nullptr) {
        memcpy(newBuffer, m_buffer, m_size);
        delete[] m_buffer;
    }

    m_buffer = newBuffer;
    m_capacity = newCapacity;
}
void BinaryWriter::WriteFloat(float data) {
    Write(&data, sizeof(float));
}

void BinaryWriter::WriteDouble(double data) {
    Write(&data, sizeof(double));
}
// д���ַ�������
void BinaryWriter::WriteStringVector(const std::vector<std::string>& strings) {
    // ��д��������Varint32��
    WriteVarInt32(static_cast<int32_t>(strings.size()));

    // д��ÿ���ַ���
    for (const auto& str : strings) {
        WriteStringUTF(str);
    }
}

// д��NBT���ݣ��򻯰汾��ʵ����Ҫ������NBT���룩
void BinaryWriter::WriteNBTData(const std::unordered_map<std::string, std::string>& nbt) {
    // ��ʵ�֣���NBT������ֵ��д��
    // ʵ��Minecraftʹ�ø��ӵ�NBT��ʽ

    // д�볤�ȣ�-1��ʾ�а汾�ţ�
    if (nbt.empty()) {
        WriteInt16(0);  // ������
        return;
    }
    std::string nbtText;
    for (const auto& pair : nbt) {
        const std::string& key = pair.first;
        const std::string& value = pair.second;
        nbtText += key + ":" + value + ";";
    }

    // д�볤�Ⱥ��ı�
    WriteInt16(static_cast<int16_t>(nbtText.size()));
    Write(nbtText.data(), nbtText.size());
}

// д����Ʒ��ջ
void BinaryWriter::WriteItemStack(const ItemStack& item) {
    // д�������Ϣ
    WriteVarInt32(item.NetworkID);

    // ����ǿ�����Ʒ����д���������
    if (item.NetworkID == 0 || item.NetworkID == -1) {
        return;
    }

    WriteUInt16(item.Count);
    WriteVarUInt32(item.MetadataValue);
    WriteVarInt32(item.BlockRuntimeID);

    // д��NBT����
    BinaryWriter nbtWriter;
    nbtWriter.WriteNBTData(item.NBTData);
    nbtWriter.WriteStringVector(item.CanBePlacedOn);
    nbtWriter.WriteStringVector(item.CanBreak);

    // ��NBT������Ϊ�ֽ���Ƭд��
    auto nbtData = nbtWriter.vect();
    WriteVarInt32(static_cast<int32_t>(nbtData.size()));
    Write(nbtData.data(), nbtData.size());
}

// д����Ʒʵ��
void BinaryWriter::WriteItemInstance(const ItemInstance& item) {
    // д���ջ��Ϣ
    WriteItemStack(item.Stack);

    // �����Ʒ�ǿ�����ֱ�ӷ���
    if (item.Stack.IsAir()) {
        return;
    }

    // д���Ƿ�������ID
    WriteUInt8(item.HasNetworkID);

    if (item.HasNetworkID) {
        WriteVarInt(item.StackNetworkID);
    }
}

// ��������ʵ��
void BinaryWriter::WriteVarInt32(int32_t value) {
    WriteVarInt(static_cast<int64_t>(value));
}

void BinaryWriter::WriteVarUInt32(uint32_t value) {
    WriteVarUInt(static_cast<uint64_t>(value));
}

void BinaryWriter::WriteBool(bool value) {
    WriteUInt8(value ? 1 : 0);
}

void BinaryWriter::WriteByteSlice(const uint8_t* data, size_t size) {
    WriteVarUInt32(static_cast<int32_t>(size));
    Write(data, size);
}

void BinaryWriter::WriteStringUTF(const std::string& str) {
    if (str.empty()) {
        WriteBool(false);
        return;
    }
    WriteVarUInt32(static_cast<int32_t>(str.size()));
    Write(str.data(), str.size());
}