#include "Tag.h"

#include <iomanip>
#include <limits>
#include <sstream>

namespace nbt {
namespace {

std::string quote(std::string_view value)
{
    std::ostringstream out;
    out << '"';
    for (unsigned char ch : value) {
        switch (ch) {
        case '"': out << "\\\""; break;
        case '\\': out << "\\\\"; break;
        case '\b': out << "\\b"; break;
        case '\f': out << "\\f"; break;
        case '\n': out << "\\n"; break;
        case '\r': out << "\\r"; break;
        case '\t': out << "\\t"; break;
        default:
            if (ch < 0x20) {
                out << "\\u00" << std::hex << std::setw(2) << std::setfill('0')
                    << static_cast<unsigned int>(ch) << std::dec;
            } else {
                out << static_cast<char>(ch);
            }
            break;
        }
    }
    out << '"';
    return out.str();
}

template <typename T>
std::string floatingPoint(T value)
{
    std::ostringstream out;
    out << std::setprecision(std::numeric_limits<T>::max_digits10) << value;
    return out.str();
}

std::string scalarString(const Tag& tag)
{
    switch (tag.type()) {
    case Type::End: return "";
    case Type::Byte: return std::to_string(static_cast<int>(*tag.getIf<int8_t>()));
    case Type::Short: return std::to_string(*tag.getIf<int16_t>());
    case Type::Int: return std::to_string(*tag.getIf<int32_t>());
    case Type::Int64: return std::to_string(*tag.getIf<int64_t>());
    case Type::Float: return floatingPoint(*tag.getIf<float>());
    case Type::Double: return floatingPoint(*tag.getIf<double>());
    case Type::String: return *tag.getIf<std::string>();
    default: return tag.toSnbt();
    }
}

void flatten(const Tag& tag, const std::string& path, std::map<std::string, std::string>& output)
{
    if (tag.type() == Type::Compound) {
        const auto& compound = *tag.getIf<Tag::Compound>();
        if (compound.empty() && !path.empty()) {
            output[path] = "{}";
        }
        for (const auto& entry : compound) {
            const std::string childPath = path.empty() ? entry.first : path + "." + entry.first;
            flatten(entry.second, childPath, output);
        }
        return;
    }

    if (tag.type() == Type::List) {
        const auto& list = *tag.getIf<Tag::List>();
        const std::string listPath = path.empty() ? "value" : path;
        output[listPath] = tag.toSnbt();
        for (size_t i = 0; i < list.size(); ++i) {
            if (list[i].type() == Type::Compound || list[i].type() == Type::List) {
                flatten(list[i], listPath + "[" + std::to_string(i) + "]", output);
            }
        }
        return;
    }

    output[path.empty() ? "value" : path] = scalarString(tag);
}

} // namespace

Tag::Tag(Type type, Value value, Type listElementType)
    : m_type(type), m_listElementType(listElementType), m_value(std::move(value))
{}

Tag Tag::FromByte(int8_t value) { return Tag(Type::Byte, value); }
Tag Tag::FromShort(int16_t value) { return Tag(Type::Short, value); }
Tag Tag::FromInt(int32_t value) { return Tag(Type::Int, value); }
Tag Tag::FromInt64(int64_t value) { return Tag(Type::Int64, value); }
Tag Tag::FromFloat(float value) { return Tag(Type::Float, value); }
Tag Tag::FromDouble(double value) { return Tag(Type::Double, value); }
Tag Tag::FromByteArray(ByteArray value) { return Tag(Type::ByteArray, std::move(value)); }
Tag Tag::FromString(std::string value) { return Tag(Type::String, std::move(value)); }
Tag Tag::FromList(Type elementType, List value) { return Tag(Type::List, std::move(value), elementType); }
Tag Tag::FromCompound(Compound value) { return Tag(Type::Compound, std::move(value)); }
Tag Tag::FromIntArray(IntArray value) { return Tag(Type::IntArray, std::move(value)); }
Tag Tag::FromLongArray(LongArray value) { return Tag(Type::LongArray, std::move(value)); }

bool Tag::isScalar() const noexcept
{
    return m_type >= Type::Byte && m_type <= Type::String && m_type != Type::ByteArray;
}

size_t Tag::size() const noexcept
{
    switch (m_type) {
    case Type::ByteArray: return getIf<ByteArray>()->size();
    case Type::String: return getIf<std::string>()->size();
    case Type::List: return getIf<List>()->size();
    case Type::Compound: return getIf<Compound>()->size();
    case Type::IntArray: return getIf<IntArray>()->size();
    case Type::LongArray: return getIf<LongArray>()->size();
    case Type::End: return 0;
    default: return 1;
    }
}

const Tag* Tag::find(std::string_view key) const noexcept
{
    const auto* compound = getIf<Compound>();
    if (!compound) return nullptr;
    auto it = compound->find(key);
    return it == compound->end() ? nullptr : &it->second;
}

Tag* Tag::find(std::string_view key) noexcept
{
    return const_cast<Tag*>(static_cast<const Tag&>(*this).find(key));
}

std::string Tag::toSnbt() const
{
    switch (m_type) {
    case Type::End:
        return "END";
    case Type::Byte:
        return std::to_string(static_cast<int>(*getIf<int8_t>())) + "b";
    case Type::Short:
        return std::to_string(*getIf<int16_t>()) + "s";
    case Type::Int:
        return std::to_string(*getIf<int32_t>());
    case Type::Int64:
        return std::to_string(*getIf<int64_t>()) + "L";
    case Type::Float:
        return floatingPoint(*getIf<float>()) + "f";
    case Type::Double:
        return floatingPoint(*getIf<double>()) + "d";
    case Type::ByteArray: {
        std::string result = "[B;";
        const auto& values = *getIf<ByteArray>();
        for (size_t i = 0; i < values.size(); ++i) {
            if (i) result += ',';
            result += std::to_string(static_cast<int>(values[i])) + "b";
        }
        return result + ']';
    }
    case Type::String:
        return quote(*getIf<std::string>());
    case Type::List: {
        std::string result = "[";
        const auto& values = *getIf<List>();
        for (size_t i = 0; i < values.size(); ++i) {
            if (i) result += ',';
            result += values[i].toSnbt();
        }
        return result + ']';
    }
    case Type::Compound: {
        std::string result = "{";
        size_t index = 0;
        for (const auto& entry : *getIf<Compound>()) {
            if (index++) result += ',';
            result += quote(entry.first) + ':' + entry.second.toSnbt();
        }
        return result + '}';
    }
    case Type::IntArray: {
        std::string result = "[I;";
        const auto& values = *getIf<IntArray>();
        for (size_t i = 0; i < values.size(); ++i) {
            if (i) result += ',';
            result += std::to_string(values[i]);
        }
        return result + ']';
    }
    case Type::LongArray: {
        std::string result = "[L;";
        const auto& values = *getIf<LongArray>();
        for (size_t i = 0; i < values.size(); ++i) {
            if (i) result += ',';
            result += std::to_string(values[i]) + "L";
        }
        return result + ']';
    }
    }
    return "";
}

void Flatten(const Tag& tag, std::map<std::string, std::string>& output)
{
    output.clear();
    flatten(tag, "", output);
}

} // namespace nbt
