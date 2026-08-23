#include "NbtPython.h"

#include "ErrorPython.h"
#include "NBT/NbtIo.h"
#include <pybind11/embed.h>
#include <pybind11/stl.h>

#include <limits>
#include <stdexcept>

namespace py = pybind11;

namespace {

struct NbtDocument {
    nbt::Tag root;
    std::string rootName;
    size_t bytesRead = 0;
};

nbt::Encoding parseEncoding(const std::string& value)
{
    if (value == "big_endian" || value == "big") return nbt::Encoding::BigEndian;
    if (value == "little_endian" || value == "little") return nbt::Encoding::LittleEndian;
    if (value == "network_little_endian" || value == "network") {
        return nbt::Encoding::NetworkLittleEndian;
    }
    throw py::value_error("encoding must be 'big_endian', 'little_endian', or 'network_little_endian'");
}

py::object tagValue(const nbt::Tag& tag)
{
    switch (tag.type()) {
    case nbt::Type::End: return py::none();
    case nbt::Type::Byte: return py::int_(*tag.getIf<int8_t>());
    case nbt::Type::Short: return py::int_(*tag.getIf<int16_t>());
    case nbt::Type::Int: return py::int_(*tag.getIf<int32_t>());
    case nbt::Type::Int64: return py::int_(*tag.getIf<int64_t>());
    case nbt::Type::Float: return py::float_(*tag.getIf<float>());
    case nbt::Type::Double: return py::float_(*tag.getIf<double>());
    case nbt::Type::ByteArray: {
        const auto& value = *tag.getIf<nbt::Tag::ByteArray>();
        return py::bytes(reinterpret_cast<const char*>(value.data()), value.size());
    }
    case nbt::Type::String: return py::str(*tag.getIf<std::string>());
    case nbt::Type::List: {
        py::list output;
        for (const auto& value : *tag.getIf<nbt::Tag::List>()) output.append(py::cast(value));
        return std::move(output);
    }
    case nbt::Type::Compound: {
        py::dict output;
        for (const auto& entry : *tag.getIf<nbt::Tag::Compound>()) {
            output[py::str(entry.first)] = py::cast(entry.second);
        }
        return std::move(output);
    }
    case nbt::Type::IntArray: return py::cast(*tag.getIf<nbt::Tag::IntArray>());
    case nbt::Type::LongArray: return py::cast(*tag.getIf<nbt::Tag::LongArray>());
    }
    return py::none();
}

py::object tagToNative(const nbt::Tag& tag)
{
    if (tag.type() == nbt::Type::List) {
        py::list output;
        for (const auto& value : *tag.getIf<nbt::Tag::List>()) output.append(tagToNative(value));
        return std::move(output);
    }
    if (tag.type() == nbt::Type::Compound) {
        py::dict output;
        for (const auto& entry : *tag.getIf<nbt::Tag::Compound>()) {
            output[py::str(entry.first)] = tagToNative(entry.second);
        }
        return std::move(output);
    }
    return tagValue(tag);
}

nbt::Tag byteTag(int value)
{
    if (value < std::numeric_limits<int8_t>::min() || value > std::numeric_limits<int8_t>::max()) {
        throw py::value_error("Byte value must be between -128 and 127");
    }
    return nbt::Tag::FromByte(static_cast<int8_t>(value));
}

nbt::Tag shortTag(int value)
{
    if (value < std::numeric_limits<int16_t>::min() || value > std::numeric_limits<int16_t>::max()) {
        throw py::value_error("Short value must be between -32768 and 32767");
    }
    return nbt::Tag::FromShort(static_cast<int16_t>(value));
}

nbt::Tag byteArrayTag(const py::bytes& value)
{
    const std::string bytes = value;
    nbt::Tag::ByteArray output(bytes.size());
    for (size_t i = 0; i < bytes.size(); ++i) output[i] = static_cast<int8_t>(bytes[i]);
    return nbt::Tag::FromByteArray(std::move(output));
}

nbt::Tag listTag(nbt::Type elementType, nbt::Tag::List values)
{
    if (elementType == nbt::Type::End && !values.empty()) {
        throw py::value_error("A non-empty List cannot use End as its element type");
    }
    for (const nbt::Tag& value : values) {
        if (value.type() != elementType) {
            throw py::type_error("All List values must match element_type");
        }
    }
    return nbt::Tag::FromList(elementType, std::move(values));
}

nbt::Tag compoundTag(nbt::Tag::Compound values)
{
    for (const auto& entry : values) {
        if (entry.second.type() == nbt::Type::End) {
            throw py::value_error("Compound tags cannot contain End values");
        }
    }
    return nbt::Tag::FromCompound(std::move(values));
}

py::object getCompoundValue(const nbt::Tag& tag, const std::string& key)
{
    const nbt::Tag* value = tag.find(key);
    return value ? py::cast(*value) : py::none();
}

py::object getListValue(const nbt::Tag& tag, size_t index)
{
    const auto* values = tag.getIf<nbt::Tag::List>();
    if (!values) throw py::type_error("Get(index) requires a List tag");
    if (index >= values->size()) return py::none();
    return py::cast((*values)[index]);
}

void setCompoundValue(nbt::Tag& tag, const std::string& key, const nbt::Tag& value)
{
    auto* compound = tag.getIf<nbt::Tag::Compound>();
    if (!compound) throw py::type_error("Set(key, value) requires a Compound tag");
    if (value.type() == nbt::Type::End) throw py::value_error("Compound tags cannot contain End values");
    compound->insert_or_assign(key, value);
}

void appendListValue(nbt::Tag& tag, const nbt::Tag& value)
{
    auto* list = tag.getIf<nbt::Tag::List>();
    if (!list) throw py::type_error("Append(value) requires a List tag");
    if (value.type() != tag.listElementType()) throw py::type_error("List values must match GetListElementType()");
    list->push_back(value);
}

NbtDocument loads(const py::bytes& data, const std::string& encoding, bool allowTrailingData,
                  size_t maxDepth, size_t maxElements)
{
    const std::string bytes = data;
    nbt::ParseOptions options;
    options.encoding = parseEncoding(encoding);
    options.allowTrailingData = allowTrailingData;
    options.limits.maxDepth = maxDepth;
    options.limits.maxElements = maxElements;
    options.limits.maxCollectionLength = maxElements;

    nbt::ParseResult result = nbt::Parse(
        reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size(), options);
    if (!result) {
        SetApiError(ApiNbtError, result.error.message + " at byte " + std::to_string(result.error.offset));
        throw py::error_already_set();
    }
    return { std::move(result.root), std::move(result.rootName), result.bytesRead };
}

py::bytes dumps(const nbt::Tag& root, const std::string& rootName, const std::string& encoding)
{
    nbt::WriteResult result = nbt::Write(root, rootName, parseEncoding(encoding));
    if (!result) {
        SetApiError(ApiNbtError, result.error);
        throw py::error_already_set();
    }
    return py::bytes(reinterpret_cast<const char*>(result.data.data()), result.data.size());
}

} // namespace

PyObject* NbtTagToPythonObject(const nbt::Tag& tag)
{
    try {
        return py::cast(tag).release().ptr();
    } catch (const py::error_already_set&) {
        return nullptr;
    }
}

PYBIND11_MODULE(nbt, module)
{
    module.doc() = "Typed Bedrock NBT parsing and serialization";

    py::enum_<nbt::Type>(module, "TagType")
        .value("End", nbt::Type::End)
        .value("Byte", nbt::Type::Byte)
        .value("Short", nbt::Type::Short)
        .value("Int", nbt::Type::Int)
        .value("Long", nbt::Type::Int64)
        .value("Float", nbt::Type::Float)
        .value("Double", nbt::Type::Double)
        .value("ByteArray", nbt::Type::ByteArray)
        .value("String", nbt::Type::String)
        .value("List", nbt::Type::List)
        .value("Compound", nbt::Type::Compound)
        .value("IntArray", nbt::Type::IntArray)
        .value("LongArray", nbt::Type::LongArray);

    py::class_<nbt::Tag>(module, "Tag")
        .def(py::init<>())
        .def_static("Byte", &byteTag)
        .def_static("Short", &shortTag)
        .def_static("Int", &nbt::Tag::FromInt)
        .def_static("Long", &nbt::Tag::FromInt64)
        .def_static("Float", &nbt::Tag::FromFloat)
        .def_static("Double", &nbt::Tag::FromDouble)
        .def_static("ByteArray", &byteArrayTag)
        .def_static("String", &nbt::Tag::FromString)
        .def_static("List", &listTag, py::arg("element_type"), py::arg("values") = nbt::Tag::List{})
        .def_static("Compound", &compoundTag, py::arg("values") = nbt::Tag::Compound{})
        .def_static("IntArray", &nbt::Tag::FromIntArray)
        .def_static("LongArray", &nbt::Tag::FromLongArray)
        .def("GetType", &nbt::Tag::type)
        .def("GetListElementType", &nbt::Tag::listElementType)
        .def("GetValue", &tagValue)
        .def("ToPython", &tagToNative)
        .def("Get", py::overload_cast<const nbt::Tag&, const std::string&>(&getCompoundValue), py::arg("key"))
        .def("Get", py::overload_cast<const nbt::Tag&, size_t>(&getListValue), py::arg("index"))
        .def("Set", &setCompoundValue, py::arg("key"), py::arg("value"))
        .def("Append", &appendListValue, py::arg("value"))
        .def("Contains", [](const nbt::Tag& tag, const std::string& key) { return tag.find(key) != nullptr; })
        .def("Erase", [](nbt::Tag& tag, const std::string& key) {
            auto* compound = tag.getIf<nbt::Tag::Compound>();
            if (!compound) throw py::type_error("Erase(key) requires a Compound tag");
            return compound->erase(key) != 0;
        })
        .def("Size", &nbt::Tag::size)
        .def("ToSnbt", &nbt::Tag::toSnbt)
        .def("__len__", &nbt::Tag::size)
        .def("__repr__", [](const nbt::Tag& tag) { return "Tag(" + tag.toSnbt() + ")"; });

    py::class_<NbtDocument>(module, "Document")
        .def("GetRoot", [](const NbtDocument& document) { return document.root; })
        .def("SetRoot", [](NbtDocument& document, const nbt::Tag& root) { document.root = root; })
        .def("GetRootName", [](const NbtDocument& document) { return document.rootName; })
        .def("SetRootName", [](NbtDocument& document, std::string name) { document.rootName = std::move(name); })
        .def("GetBytesRead", [](const NbtDocument& document) { return document.bytesRead; })
        .def("ToSnbt", [](const NbtDocument& document) { return document.root.toSnbt(); });

    module.attr("NbtError") = py::reinterpret_borrow<py::object>(ApiNbtError);
    module.def("loads", &loads, py::arg("data"), py::arg("encoding") = "network_little_endian",
               py::arg("allow_trailing_data") = false, py::arg("max_depth") = 64,
               py::arg("max_elements") = 1 << 20);
    module.def("load_tag", [](const py::bytes& data, const std::string& encoding) {
        return loads(data, encoding, false, 64, 1 << 20).root;
    }, py::arg("data"), py::arg("encoding") = "network_little_endian");
    module.def("dumps", &dumps, py::arg("root"), py::arg("root_name") = "",
               py::arg("encoding") = "network_little_endian");
    module.def("dump_document", [](const NbtDocument& document, const std::string& encoding) {
        return dumps(document.root, document.rootName, encoding);
    }, py::arg("document"), py::arg("encoding") = "network_little_endian");
    module.def("flatten", [](const nbt::Tag& tag) {
        std::map<std::string, std::string> output;
        nbt::Flatten(tag, output);
        return output;
    });
}
