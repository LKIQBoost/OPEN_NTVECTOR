#include "NbtIo.h"

#include <cstring>
#include <limits>
#include <type_traits>
#include <utility>

namespace nbt {
namespace {

class Reader {
public:
    Reader(const uint8_t* data, size_t size, const ParseOptions& options)
        : m_data(data), m_size(size), m_options(options)
    {}

    ParseResult parse()
    {
        ParseResult result;
        Type rootType = Type::End;
        if (!readType(rootType)) return finishError();

        if (rootType != Type::End && !readString(result.rootName)) return finishError();
        if (!parsePayload(rootType, 0, result.root)) return finishError();

        result.bytesRead = m_pos;
        if (!m_options.allowTrailingData && m_pos != m_size) {
            fail(ParseErrorCode::TrailingData, "trailing bytes after root tag");
            return finishError();
        }

        result.success = true;
        return result;
    }

private:
    ParseResult finishError() const
    {
        ParseResult result;
        result.bytesRead = m_pos;
        result.error = m_error;
        return result;
    }

    bool fail(ParseErrorCode code, const char* message)
    {
        if (m_error.code == ParseErrorCode::None) {
            m_error.code = code;
            m_error.offset = m_pos;
            m_error.message = message;
        }
        return false;
    }

    bool readBytes(size_t count, const uint8_t*& output)
    {
        if (!m_data || count > m_size - m_pos) {
            return fail(ParseErrorCode::UnexpectedEnd, "unexpected end of NBT data");
        }
        output = m_data + m_pos;
        m_pos += count;
        return true;
    }

    bool readU8(uint8_t& value)
    {
        const uint8_t* bytes = nullptr;
        if (!readBytes(1, bytes)) return false;
        value = bytes[0];
        return true;
    }

    bool readUnsigned(size_t width, uint64_t& value)
    {
        const uint8_t* bytes = nullptr;
        if (!readBytes(width, bytes)) return false;
        value = 0;
        if (m_options.encoding == Encoding::BigEndian) {
            for (size_t i = 0; i < width; ++i) value = (value << 8) | bytes[i];
        } else {
            for (size_t i = 0; i < width; ++i) value |= static_cast<uint64_t>(bytes[i]) << (i * 8);
        }
        return true;
    }

    bool readVarUInt(uint64_t& value, size_t maxBytes)
    {
        value = 0;
        for (size_t i = 0; i < maxBytes; ++i) {
            uint8_t byte = 0;
            if (!readU8(byte)) return false;
            if (i == 9 && (byte & 0xFE) != 0) {
                return fail(ParseErrorCode::InvalidLength, "varint exceeds 64-bit range");
            }
            value |= static_cast<uint64_t>(byte & 0x7F) << (i * 7);
            if ((byte & 0x80) == 0) return true;
        }
        return fail(ParseErrorCode::InvalidLength, "varint exceeds its maximum width");
    }

    bool readInt16(int16_t& value)
    {
        uint64_t raw = 0;
        if (!readUnsigned(2, raw)) return false;
        const uint16_t bits = static_cast<uint16_t>(raw);
        std::memcpy(&value, &bits, sizeof(value));
        return true;
    }

    bool readInt32(int32_t& value)
    {
        if (m_options.encoding == Encoding::NetworkLittleEndian) {
            uint64_t raw = 0;
            if (!readVarUInt(raw, 5) || raw > std::numeric_limits<uint32_t>::max()) {
                return fail(ParseErrorCode::InvalidLength, "invalid 32-bit zigzag varint");
            }
            value = static_cast<int32_t>(raw >> 1) ^ -static_cast<int32_t>(raw & 1);
            return true;
        }

        uint64_t raw = 0;
        if (!readUnsigned(4, raw)) return false;
        const uint32_t bits = static_cast<uint32_t>(raw);
        std::memcpy(&value, &bits, sizeof(value));
        return true;
    }

    bool readInt64(int64_t& value)
    {
        if (m_options.encoding == Encoding::NetworkLittleEndian) {
            uint64_t raw = 0;
            if (!readVarUInt(raw, 10)) return false;
            value = static_cast<int64_t>(raw >> 1) ^ -static_cast<int64_t>(raw & 1);
            return true;
        }

        uint64_t raw = 0;
        if (!readUnsigned(8, raw)) return false;
        std::memcpy(&value, &raw, sizeof(value));
        return true;
    }

    bool readFloat(float& value)
    {
        uint64_t raw = 0;
        if (!readUnsigned(4, raw)) return false;
        const uint32_t bits = static_cast<uint32_t>(raw);
        std::memcpy(&value, &bits, sizeof(value));
        return true;
    }

    bool readDouble(double& value)
    {
        uint64_t raw = 0;
        if (!readUnsigned(8, raw)) return false;
        std::memcpy(&value, &raw, sizeof(value));
        return true;
    }

    bool readString(std::string& value)
    {
        uint64_t length = 0;
        if (m_options.encoding == Encoding::NetworkLittleEndian) {
            if (!readVarUInt(length, 5) || length > std::numeric_limits<uint32_t>::max()) {
                return fail(ParseErrorCode::InvalidLength, "invalid NBT string length");
            }
        } else if (!readUnsigned(2, length)) {
            return false;
        }

        if (length > m_options.limits.maxStringBytes) {
            return fail(ParseErrorCode::StringLimit, "NBT string exceeds configured limit");
        }

        const uint8_t* bytes = nullptr;
        if (!readBytes(static_cast<size_t>(length), bytes)) return false;
        value.assign(reinterpret_cast<const char*>(bytes), static_cast<size_t>(length));
        return true;
    }

    bool readLength(size_t& length)
    {
        int32_t signedLength = 0;
        if (!readInt32(signedLength)) return false;
        if (signedLength < 0) return fail(ParseErrorCode::InvalidLength, "negative NBT collection length");
        length = static_cast<size_t>(signedLength);
        if (length > m_options.limits.maxCollectionLength) {
            return fail(ParseErrorCode::ElementLimit, "NBT collection exceeds configured limit");
        }
        if (length > m_options.limits.maxElements - m_totalElements) {
            return fail(ParseErrorCode::ElementLimit, "NBT document exceeds configured element limit");
        }
        m_totalElements += length;
        return true;
    }

    bool readType(Type& type)
    {
        uint8_t raw = 0;
        if (!readU8(raw)) return false;
        if (raw > static_cast<uint8_t>(Type::LongArray)) {
            return fail(ParseErrorCode::InvalidType, "unknown NBT tag type");
        }
        type = static_cast<Type>(raw);
        return true;
    }

    bool parsePayload(Type type, size_t depth, Tag& output)
    {
        if (depth > m_options.limits.maxDepth) {
            return fail(ParseErrorCode::DepthLimit, "NBT nesting exceeds configured depth limit");
        }

        switch (type) {
        case Type::End:
            output = Tag();
            return true;
        case Type::Byte: {
            uint8_t value = 0;
            if (!readU8(value)) return false;
            output = Tag::FromByte(static_cast<int8_t>(value));
            return true;
        }
        case Type::Short: {
            int16_t value = 0;
            if (!readInt16(value)) return false;
            output = Tag::FromShort(value);
            return true;
        }
        case Type::Int: {
            int32_t value = 0;
            if (!readInt32(value)) return false;
            output = Tag::FromInt(value);
            return true;
        }
        case Type::Int64: {
            int64_t value = 0;
            if (!readInt64(value)) return false;
            output = Tag::FromInt64(value);
            return true;
        }
        case Type::Float: {
            float value = 0;
            if (!readFloat(value)) return false;
            output = Tag::FromFloat(value);
            return true;
        }
        case Type::Double: {
            double value = 0;
            if (!readDouble(value)) return false;
            output = Tag::FromDouble(value);
            return true;
        }
        case Type::ByteArray: {
            size_t length = 0;
            if (!readLength(length)) return false;
            const uint8_t* bytes = nullptr;
            if (!readBytes(length, bytes)) return false;
            Tag::ByteArray values(length);
            for (size_t i = 0; i < length; ++i) values[i] = static_cast<int8_t>(bytes[i]);
            output = Tag::FromByteArray(std::move(values));
            return true;
        }
        case Type::String: {
            std::string value;
            if (!readString(value)) return false;
            output = Tag::FromString(std::move(value));
            return true;
        }
        case Type::List: {
            Type elementType = Type::End;
            size_t length = 0;
            if (!readType(elementType) || !readLength(length)) return false;
            if (elementType == Type::End && length != 0) {
                return fail(ParseErrorCode::InvalidListType, "non-empty NBT list cannot use End element type");
            }
            Tag::List values;
            values.reserve(length);
            for (size_t i = 0; i < length; ++i) {
                Tag value;
                if (!parsePayload(elementType, depth + 1, value)) return false;
                values.push_back(std::move(value));
            }
            output = Tag::FromList(elementType, std::move(values));
            return true;
        }
        case Type::Compound: {
            Tag::Compound values;
            while (true) {
                Type childType = Type::End;
                if (!readType(childType)) return false;
                if (childType == Type::End) break;

                if (m_totalElements == m_options.limits.maxElements) {
                    return fail(ParseErrorCode::ElementLimit, "NBT document exceeds configured element limit");
                }
                ++m_totalElements;

                std::string name;
                Tag child;
                if (!readString(name) || !parsePayload(childType, depth + 1, child)) return false;
                values.insert_or_assign(std::move(name), std::move(child));
            }
            output = Tag::FromCompound(std::move(values));
            return true;
        }
        case Type::IntArray: {
            size_t length = 0;
            if (!readLength(length)) return false;
            Tag::IntArray values;
            values.reserve(length);
            for (size_t i = 0; i < length; ++i) {
                int32_t value = 0;
                if (!readInt32(value)) return false;
                values.push_back(value);
            }
            output = Tag::FromIntArray(std::move(values));
            return true;
        }
        case Type::LongArray: {
            size_t length = 0;
            if (!readLength(length)) return false;
            Tag::LongArray values;
            values.reserve(length);
            for (size_t i = 0; i < length; ++i) {
                int64_t value = 0;
                if (!readInt64(value)) return false;
                values.push_back(value);
            }
            output = Tag::FromLongArray(std::move(values));
            return true;
        }
        }
        return fail(ParseErrorCode::InvalidType, "unsupported NBT tag type");
    }

    const uint8_t* m_data = nullptr;
    size_t m_size = 0;
    size_t m_pos = 0;
    size_t m_totalElements = 0;
    const ParseOptions& m_options;
    ParseError m_error;
};

class Writer {
public:
    explicit Writer(Encoding encoding) : m_encoding(encoding) {}

    WriteResult write(const Tag& root, const std::string& rootName)
    {
        writeU8(static_cast<uint8_t>(root.type()));
        if (root.type() != Type::End && !writeString(rootName)) return finish();
        if (!writePayload(root)) return finish();

        WriteResult result;
        result.success = true;
        result.data = std::move(m_data);
        return result;
    }

private:
    WriteResult finish()
    {
        WriteResult result;
        result.error = std::move(m_error);
        return result;
    }

    bool fail(const char* message)
    {
        if (m_error.empty()) m_error = message;
        return false;
    }

    void writeU8(uint8_t value) { m_data.push_back(value); }

    void writeUnsigned(uint64_t value, size_t width)
    {
        if (m_encoding == Encoding::BigEndian) {
            for (size_t i = width; i > 0; --i) {
                writeU8(static_cast<uint8_t>(value >> ((i - 1) * 8)));
            }
        } else {
            for (size_t i = 0; i < width; ++i) {
                writeU8(static_cast<uint8_t>(value >> (i * 8)));
            }
        }
    }

    void writeVarUInt(uint64_t value)
    {
        do {
            uint8_t byte = static_cast<uint8_t>(value & 0x7F);
            value >>= 7;
            if (value != 0) byte |= 0x80;
            writeU8(byte);
        } while (value != 0);
    }

    void writeInt16(int16_t value)
    {
        uint16_t bits = 0;
        std::memcpy(&bits, &value, sizeof(bits));
        writeUnsigned(bits, sizeof(bits));
    }

    void writeInt32(int32_t value)
    {
        if (m_encoding == Encoding::NetworkLittleEndian) {
            const uint32_t bits = static_cast<uint32_t>(value);
            writeVarUInt((bits << 1) ^ static_cast<uint32_t>(value >> 31));
            return;
        }
        uint32_t bits = 0;
        std::memcpy(&bits, &value, sizeof(bits));
        writeUnsigned(bits, sizeof(bits));
    }

    void writeInt64(int64_t value)
    {
        if (m_encoding == Encoding::NetworkLittleEndian) {
            const uint64_t bits = static_cast<uint64_t>(value);
            writeVarUInt((bits << 1) ^ static_cast<uint64_t>(value >> 63));
            return;
        }
        uint64_t bits = 0;
        std::memcpy(&bits, &value, sizeof(bits));
        writeUnsigned(bits, sizeof(bits));
    }

    template <typename T>
    void writeFloating(T value)
    {
        using Bits = std::conditional_t<sizeof(T) == 4, uint32_t, uint64_t>;
        Bits bits = 0;
        std::memcpy(&bits, &value, sizeof(bits));
        writeUnsigned(bits, sizeof(bits));
    }

    bool writeString(const std::string& value)
    {
        if (m_encoding == Encoding::NetworkLittleEndian) {
            if (value.size() > std::numeric_limits<uint32_t>::max()) {
                return fail("NBT string exceeds the network encoding limit");
            }
            writeVarUInt(value.size());
        } else {
            if (value.size() > std::numeric_limits<uint16_t>::max()) {
                return fail("NBT string exceeds the standard encoding limit");
            }
            writeUnsigned(value.size(), 2);
        }
        m_data.insert(m_data.end(), value.begin(), value.end());
        return true;
    }

    bool writeLength(size_t length)
    {
        if (length > static_cast<size_t>(std::numeric_limits<int32_t>::max())) {
            return fail("NBT collection exceeds the 32-bit length limit");
        }
        writeInt32(static_cast<int32_t>(length));
        return true;
    }

    bool writePayload(const Tag& tag)
    {
        switch (tag.type()) {
        case Type::End:
            return true;
        case Type::Byte:
            writeU8(static_cast<uint8_t>(*tag.getIf<int8_t>()));
            return true;
        case Type::Short:
            writeInt16(*tag.getIf<int16_t>());
            return true;
        case Type::Int:
            writeInt32(*tag.getIf<int32_t>());
            return true;
        case Type::Int64:
            writeInt64(*tag.getIf<int64_t>());
            return true;
        case Type::Float:
            writeFloating(*tag.getIf<float>());
            return true;
        case Type::Double:
            writeFloating(*tag.getIf<double>());
            return true;
        case Type::ByteArray: {
            const auto& values = *tag.getIf<Tag::ByteArray>();
            if (!writeLength(values.size())) return false;
            for (int8_t value : values) writeU8(static_cast<uint8_t>(value));
            return true;
        }
        case Type::String:
            return writeString(*tag.getIf<std::string>());
        case Type::List: {
            const auto& values = *tag.getIf<Tag::List>();
            const Type elementType = tag.listElementType();
            if (!values.empty() && elementType == Type::End) {
                return fail("non-empty NBT list cannot use End element type");
            }
            for (const Tag& value : values) {
                if (value.type() != elementType) return fail("NBT list contains mixed tag types");
            }
            writeU8(static_cast<uint8_t>(elementType));
            if (!writeLength(values.size())) return false;
            for (const Tag& value : values) {
                if (!writePayload(value)) return false;
            }
            return true;
        }
        case Type::Compound:
            for (const auto& entry : *tag.getIf<Tag::Compound>()) {
                if (entry.second.type() == Type::End) {
                    return fail("NBT compound cannot contain a named End tag");
                }
                writeU8(static_cast<uint8_t>(entry.second.type()));
                if (!writeString(entry.first) || !writePayload(entry.second)) return false;
            }
            writeU8(static_cast<uint8_t>(Type::End));
            return true;
        case Type::IntArray: {
            const auto& values = *tag.getIf<Tag::IntArray>();
            if (!writeLength(values.size())) return false;
            for (int32_t value : values) writeInt32(value);
            return true;
        }
        case Type::LongArray: {
            const auto& values = *tag.getIf<Tag::LongArray>();
            if (!writeLength(values.size())) return false;
            for (int64_t value : values) writeInt64(value);
            return true;
        }
        }
        return fail("unsupported NBT tag type");
    }

    Encoding m_encoding;
    std::vector<uint8_t> m_data;
    std::string m_error;
};

} // namespace

ParseResult Parse(const uint8_t* data, size_t size, const ParseOptions& options)
{
    return Reader(data, size, options).parse();
}

ParseResult Parse(const std::vector<uint8_t>& data, const ParseOptions& options)
{
    return Parse(data.data(), data.size(), options);
}

WriteResult Write(const Tag& root, const std::string& rootName, Encoding encoding)
{
    return Writer(encoding).write(root, rootName);
}

} // namespace nbt
