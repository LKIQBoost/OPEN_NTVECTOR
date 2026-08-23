#include "BlockActorData.h"
#include "NBT/NbtIo.h"

#include <utility>

void BlockActorData::Deserializ(std::vector<unsigned char> pack)
{
    BinaryReader br(pack.data(), static_cast<int>(pack.size()));

    Position = pkt_io::ReadBlockPos(br);
    RawNBT.clear();
    NBT = nbt::Tag();
    NBTRootName.clear();
    NBTError.clear();
    NBTParsed = false;
    NBTFields.clear();

    if (br.m_pointer > pack.size()) {
        NBTError = "block position exceeds packet length";
        return;
    }

    const size_t remaining = pack.size() - br.m_pointer;
    if (remaining == 0) {
        NBTError = "missing block actor NBT payload";
        return;
    }

    RawNBT.assign(pack.begin() + static_cast<std::ptrdiff_t>(br.m_pointer), pack.end());

    nbt::ParseOptions options;
    options.encoding = nbt::Encoding::NetworkLittleEndian;
    nbt::ParseResult result = nbt::Parse(RawNBT, options);
    if (!result) {
        NBTError = result.error.message + " at byte " + std::to_string(result.error.offset);
        return;
    }

    NBT = std::move(result.root);
    NBTRootName = std::move(result.rootName);
    NBTParsed = true;
    nbt::Flatten(NBT, NBTFields);
}

bool BlockActorData::FlattenNbt(
    const uint8_t* data,
    size_t len,
    std::map<std::string, std::string>& out)
{
    nbt::ParseOptions options;
    options.encoding = nbt::Encoding::NetworkLittleEndian;
    const nbt::ParseResult result = nbt::Parse(data, len, options);
    if (!result) {
        out.clear();
        return false;
    }

    nbt::Flatten(result.root, out);
    return true;
}
