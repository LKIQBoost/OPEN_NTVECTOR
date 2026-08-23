#include "NBT/NbtIo.h"

#include <cassert>
#include <cmath>
#include <iostream>

namespace {

nbt::Tag sample()
{
    nbt::Tag::Compound nested;
    nested["name"] = nbt::Tag::FromString("container");
    nested["enabled"] = nbt::Tag::FromByte(1);

    nbt::Tag::List list;
    list.push_back(nbt::Tag::FromInt(-1));
    list.push_back(nbt::Tag::FromInt(0));
    list.push_back(nbt::Tag::FromInt(300));

    nbt::Tag::Compound root;
    root["byte"] = nbt::Tag::FromByte(-12);
    root["short"] = nbt::Tag::FromShort(-1234);
    root["int"] = nbt::Tag::FromInt(-1234567);
    root["long"] = nbt::Tag::FromInt64(-1234567890123LL);
    root["float"] = nbt::Tag::FromFloat(1.25f);
    root["double"] = nbt::Tag::FromDouble(-9.5);
    root["bytes"] = nbt::Tag::FromByteArray({ -1, 0, 1, 127 });
    root["string"] = nbt::Tag::FromString("StaticNeteaseBot");
    root["list"] = nbt::Tag::FromList(nbt::Type::Int, std::move(list));
    root["compound"] = nbt::Tag::FromCompound(std::move(nested));
    root["ints"] = nbt::Tag::FromIntArray({ -1, 0, 1, 100000 });
    root["longs"] = nbt::Tag::FromLongArray({ -1, 0, 1, 10000000000LL });
    return nbt::Tag::FromCompound(std::move(root));
}

void roundTrip(nbt::Encoding encoding)
{
    const nbt::Tag original = sample();
    const nbt::WriteResult written = nbt::Write(original, "root", encoding);
    assert(written.success);
    assert(!written.data.empty());

    nbt::ParseOptions options;
    options.encoding = encoding;
    const nbt::ParseResult parsed = nbt::Parse(written.data, options);
    assert(parsed.success);
    assert(parsed.rootName == "root");
    assert(parsed.bytesRead == written.data.size());
    assert(parsed.root.toSnbt() == original.toSnbt());
}

void invalidInputs()
{
    const std::vector<uint8_t> trailing = { 0x00, 0x7f };
    nbt::ParseOptions strict;
    strict.encoding = nbt::Encoding::NetworkLittleEndian;
    const nbt::ParseResult trailingResult = nbt::Parse(trailing, strict);
    assert(!trailingResult.success);
    assert(trailingResult.error.code == nbt::ParseErrorCode::TrailingData);

    nbt::ParseOptions permissive = strict;
    permissive.allowTrailingData = true;
    const nbt::ParseResult permissiveResult = nbt::Parse(trailing, permissive);
    assert(permissiveResult.success);
    assert(permissiveResult.bytesRead == 1);

    const std::vector<uint8_t> invalidType = { 0x7f };
    const nbt::ParseResult invalidTypeResult = nbt::Parse(invalidType, strict);
    assert(!invalidTypeResult.success);
    assert(invalidTypeResult.error.code == nbt::ParseErrorCode::InvalidType);

    nbt::ParseOptions limited = strict;
    limited.limits.maxDepth = 0;
    const nbt::ParseResult depthResult = nbt::Parse(nbt::Write(sample(), "root",
        nbt::Encoding::NetworkLittleEndian).data, limited);
    assert(!depthResult.success);
    assert(depthResult.error.code == nbt::ParseErrorCode::DepthLimit);
}

} // namespace

int main()
{
    roundTrip(nbt::Encoding::BigEndian);
    roundTrip(nbt::Encoding::LittleEndian);
    roundTrip(nbt::Encoding::NetworkLittleEndian);
    invalidInputs();
    std::cout << "NBT round-trip tests passed\n";
    return 0;
}
