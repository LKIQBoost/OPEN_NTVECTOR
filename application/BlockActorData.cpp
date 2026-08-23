#include "BlockActorData.h"
#include <cstring>

namespace {

// Minimal bounds-checked cursor over a binary NBT payload.
// Bedrock block-entity NBT ("nbt" native type = prismarine-nbt littleVarint):
//   - little-endian numerics
//   - string / tag-name lengths: unsigned varint (LEB128)
//   - TAG_INT: zigzag32, TAG_LONG: zigzag64 (signed varints)
//   - list / array element counts: zigzag32
struct NbtCursor {
    const uint8_t* data;
    size_t size;
    size_t pos = 0;
    bool ok = true;

    explicit NbtCursor(const uint8_t* d, size_t n) : data(d), size(n) {}

    size_t remaining() const { return size - pos; }

    bool readBytes(size_t n, const uint8_t*& out) {
        if (!ok || n > remaining()) { ok = false; return false; }
        out = data + pos;
        pos += n;
        return true;
    }

    bool readU8(uint8_t& v) {
        const uint8_t* p;
        if (!readBytes(1, p)) return false;
        v = p[0];
        return true;
    }

    bool readU16(uint16_t& v) {
        const uint8_t* p;
        if (!readBytes(2, p)) return false;
        v = static_cast<uint16_t>(p[0] | (p[1] << 8));
        return true;
    }

    // unsigned LEB128 (up to 5 bytes)
    bool readUnsignedVarInt(int32_t& v) {
        uint32_t result = 0;
        unsigned shift = 0;
        for (int i = 0; i < 5; ++i) {
            uint8_t b = 0;
            if (!readU8(b)) return false;
            result |= static_cast<uint32_t>(b & 0x7F) << shift;
            if ((b & 0x80) == 0) {
                v = static_cast<int32_t>(result);
                return true;
            }
            shift += 7;
        }
        ok = false;
        return false;
    }

    // signed varint with zigzag (zigzag32)
    bool readZigZag32(int32_t& v) {
        int32_t raw = 0;
        if (!readUnsignedVarInt(raw)) return false;
        v = static_cast<int32_t>((static_cast<uint32_t>(raw) >> 1) ^
                                 static_cast<uint32_t>(-static_cast<int32_t>(raw & 1)));
        return true;
    }

    // signed varint with zigzag (zigzag64)
    bool readZigZag64(int64_t& v) {
        uint64_t raw = 0;
        unsigned shift = 0;
        for (int i = 0; i < 10; ++i) {
            uint8_t b = 0;
            if (!readU8(b)) return false;
            raw |= static_cast<uint64_t>(b & 0x7F) << shift;
            if ((b & 0x80) == 0) {
                v = static_cast<int64_t>((raw >> 1) ^ static_cast<uint64_t>(-static_cast<int64_t>(raw & 1)));
                return true;
            }
            shift += 7;
        }
        ok = false;
        return false;
    }

    bool readF32(float& v) {
        const uint8_t* p;
        if (!readBytes(4, p)) return false;
        uint32_t u = static_cast<uint32_t>(p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24));
        memcpy(&v, &u, sizeof(v));
        return true;
    }

    bool readF64(double& v) {
        const uint8_t* p;
        if (!readBytes(8, p)) return false;
        uint64_t u = 0;
        for (int i = 7; i >= 0; --i) u = (u << 8) | p[i];
        memcpy(&v, &u, sizeof(v));
        return true;
    }

    bool readString(std::string& s) {
        int32_t n = 0;
        if (!readUnsignedVarInt(n) || n < 0) return false;
        const uint8_t* p;
        if (!readBytes(static_cast<size_t>(n), p)) return false;
        s.assign(reinterpret_cast<const char*>(p), static_cast<size_t>(n));
        return true;
    }
};

// NBT tag type ids
enum NbtTag : uint8_t {
    TAG_END = 0,
    TAG_BYTE = 1,
    TAG_SHORT = 2,
    TAG_INT = 3,
    TAG_LONG = 4,
    TAG_FLOAT = 5,
    TAG_DOUBLE = 6,
    TAG_BYTE_ARRAY = 7,
    TAG_STRING = 8,
    TAG_LIST = 9,
    TAG_COMPOUND = 10,
    TAG_INT_ARRAY = 11,
    TAG_LONG_ARRAY = 12
};

constexpr size_t kMaxDepth = 6;
constexpr size_t kMaxEntries = 512;

// Parse a single value of the given tag type; compound/list recurse.
bool parseValue(NbtCursor& c, uint8_t type, const std::string& key,
                std::map<std::string, std::string>& out, size_t depth) {
    if (depth > kMaxDepth || out.size() > kMaxEntries) return false;

    switch (type) {
    case TAG_BYTE: {
        uint8_t v = 0;
        if (!c.readU8(v)) return false;
        out[key] = std::to_string(static_cast<int32_t>(static_cast<int8_t>(v)));
        return true;
    }
    case TAG_SHORT: {
        uint16_t v = 0;
        if (!c.readU16(v)) return false;
        out[key] = std::to_string(static_cast<int32_t>(static_cast<int16_t>(v)));
        return true;
    }
    case TAG_INT: {
        int32_t v = 0;
        if (!c.readZigZag32(v)) return false;
        out[key] = std::to_string(v);
        return true;
    }
    case TAG_LONG: {
        int64_t v = 0;
        if (!c.readZigZag64(v)) return false;
        out[key] = std::to_string(v);
        return true;
    }
    case TAG_FLOAT: {
        float v = 0;
        if (!c.readF32(v)) return false;
        out[key] = std::to_string(v);
        return true;
    }
    case TAG_DOUBLE: {
        double v = 0;
        if (!c.readF64(v)) return false;
        out[key] = std::to_string(v);
        return true;
    }
    case TAG_BYTE_ARRAY: {
        int32_t n = 0;
        if (!c.readZigZag32(n) || n < 0) return false;
        const uint8_t* p;
        if (!c.readBytes(static_cast<size_t>(n), p)) return false;
        out[key] = std::to_string(n) + " bytes";
        return true;
    }
    case TAG_STRING: {
        std::string s;
        if (!c.readString(s)) return false;
        out[key] = s;
        return true;
    }
    case TAG_LIST: {
        uint8_t elemType = 0;
        int32_t n = 0;
        if (!c.readU8(elemType) || !c.readZigZag32(n) || n < 0) return false;
        if (elemType == TAG_COMPOUND || elemType == TAG_LIST) {
            out[key] = std::to_string(n) + " items";
            for (int32_t i = 0; i < n && c.ok; ++i) {
                std::string dummy = key + ".[" + std::to_string(i) + "]";
                parseValue(c, elemType, dummy, out, depth + 1);
            }
            return true;
        }
        // scalar list -> join values
        std::string joined;
        for (int32_t i = 0; i < n && c.ok; ++i) {
            std::string itemKey = key + ".[" + std::to_string(i) + "]";
            if (!parseValue(c, elemType, itemKey, out, depth + 1)) break;
            if (!joined.empty()) joined += ",";
            joined += out[itemKey];
            out.erase(itemKey);
            if (i >= 32) { joined += ",..."; break; }
        }
        if (!joined.empty()) out[key] = joined;
        else out[key] = "0 items";
        return true;
    }
    case TAG_COMPOUND: {
        // named tags until END
        while (c.ok) {
            uint8_t t = 0;
            if (!c.readU8(t)) return false;
            if (t == TAG_END) return true;
            std::string name;
            if (!c.readString(name)) return false;
            std::string childKey = key.empty() ? name : key + "." + name;
            if (!parseValue(c, t, childKey, out, depth + 1)) return false;
        }
        return c.ok;
    }
    case TAG_INT_ARRAY: {
        int32_t n = 0;
        if (!c.readZigZag32(n) || n < 0) return false;
        const uint8_t* p;
        if (!c.readBytes(static_cast<size_t>(n) * 4, p)) return false;
        out[key] = std::to_string(n) + " items";
        return true;
    }
    case TAG_LONG_ARRAY: {
        int32_t n = 0;
        if (!c.readZigZag32(n) || n < 0) return false;
        const uint8_t* p;
        if (!c.readBytes(static_cast<size_t>(n) * 8, p)) return false;
        out[key] = std::to_string(n) + " items";
        return true;
    }
    default:
        return false;
    }
}

// Parse a named tag (type + name + value).
bool parseNamedTag(NbtCursor& c, std::map<std::string, std::string>& out) {
    uint8_t type = 0;
    if (!c.readU8(type)) return false;
    if (type == TAG_END) return true;

    std::string name;
    if (!c.readString(name)) return false;

    // A non-empty root name (e.g. the block entity type) is exposed as "id".
    if (name.empty() || type != TAG_COMPOUND) {
        return parseValue(c, type, name, out, 0);
    }

    if (!name.empty() && out.find("id") == out.end()) {
        out["id"] = name;
    }
    return parseValue(c, TAG_COMPOUND, "", out, 0);
}

} // namespace

void BlockActorData::Deserializ(std::vector<unsigned char> pack)
{
    BinaryReader br(pack.data(), static_cast<int>(pack.size()));

    // position (BlockCoordinates) then NBT occupying the rest of the packet
    Position = pkt_io::ReadBlockPos(br);
    RawNBT.clear();
    NBTFields.clear();

    size_t remaining = static_cast<size_t>(pack.size()) - br.m_pointer;
    if (remaining > 0 && remaining < (1 << 24)) {
        const uint8_t* p = reinterpret_cast<const uint8_t*>(br.Read(remaining));
        if (p) {
            RawNBT.assign(p, p + remaining);
            FlattenNbt(p, remaining, NBTFields);
        }
    }
}

bool BlockActorData::FlattenNbt(const uint8_t* data, size_t len, std::map<std::string, std::string>& out)
{
    if (!data || len == 0) return false;
    out.clear();
    NbtCursor c(data, len);
    bool parsed = parseNamedTag(c, out);
    return parsed && !out.empty();
}
