#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace nbt {

enum class Type : uint8_t {
    End = 0,
    Byte = 1,
    Short = 2,
    Int = 3,
    Int64 = 4,
    Float = 5,
    Double = 6,
    ByteArray = 7,
    String = 8,
    List = 9,
    Compound = 10,
    IntArray = 11,
    LongArray = 12,
};

class Tag {
public:
    using ByteArray = std::vector<int8_t>;
    using List = std::vector<Tag>;
    using Compound = std::map<std::string, Tag, std::less<>>;
    using IntArray = std::vector<int32_t>;
    using LongArray = std::vector<int64_t>;
    using Value = std::variant<
        std::monostate,
        int8_t,
        int16_t,
        int32_t,
        int64_t,
        float,
        double,
        ByteArray,
        std::string,
        List,
        Compound,
        IntArray,
        LongArray>;

    Tag() = default;

    static Tag FromByte(int8_t value);
    static Tag FromShort(int16_t value);
    static Tag FromInt(int32_t value);
    static Tag FromInt64(int64_t value);
    static Tag FromFloat(float value);
    static Tag FromDouble(double value);
    static Tag FromByteArray(ByteArray value);
    static Tag FromString(std::string value);
    static Tag FromList(Type elementType, List value);
    static Tag FromCompound(Compound value);
    static Tag FromIntArray(IntArray value);
    static Tag FromLongArray(LongArray value);

    Type type() const noexcept { return m_type; }
    Type listElementType() const noexcept { return m_listElementType; }
    bool is(Type type) const noexcept { return m_type == type; }
    bool isScalar() const noexcept;
    size_t size() const noexcept;
    bool empty() const noexcept { return size() == 0; }

    template <typename T>
    const T* getIf() const noexcept
    {
        return std::get_if<T>(&m_value);
    }

    template <typename T>
    T* getIf() noexcept
    {
        return std::get_if<T>(&m_value);
    }

    const Tag* find(std::string_view key) const noexcept;
    Tag* find(std::string_view key) noexcept;

    std::string toSnbt() const;

private:
    Tag(Type type, Value value, Type listElementType = Type::End);

    Type m_type = Type::End;
    Type m_listElementType = Type::End;
    Value m_value;
};

void Flatten(const Tag& tag, std::map<std::string, std::string>& output);

} // namespace nbt
