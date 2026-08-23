#include "BlockDataStore.h"

std::mutex BlockDataStore::s_mutex;
std::unordered_map<int64_t, std::pair<std::vector<uint8_t>, std::map<std::string, std::string>>> BlockDataStore::s_blockActors;
std::unordered_map<uint8_t, ContainerOpenInfo> BlockDataStore::s_openInfo;
std::unordered_map<uint8_t, std::vector<SlotItem>> BlockDataStore::s_contents;
std::unordered_map<int64_t, uint8_t> BlockDataStore::s_posToWindow;

int64_t BlockDataStore::PackPos(const BlockPos& p)
{
    // Minecraft-style block position packing (28/12/24 bits)
    return (static_cast<int64_t>(p.x & 0x3FFFFFF) << 38) |
           (static_cast<int64_t>(p.z & 0x3FFFFFF) << 12) |
           static_cast<int64_t>(p.y & 0xFFF);
}

void BlockDataStore::StoreBlockActor(const BlockPos& pos,
                                     const std::vector<uint8_t>& rawNbt,
                                     const std::map<std::string, std::string>& fields)
{
    std::lock_guard<std::mutex> lock(s_mutex);
    s_blockActors[PackPos(pos)] = { rawNbt, fields };
}

void BlockDataStore::StoreContainerOpen(const ContainerOpenInfo& info)
{
    std::lock_guard<std::mutex> lock(s_mutex);
    s_openInfo[info.WindowID] = info;
    s_posToWindow[PackPos(info.Position)] = info.WindowID;
}

void BlockDataStore::StoreContainerContent(uint8_t windowId, const std::vector<SlotItem>& slots)
{
    std::lock_guard<std::mutex> lock(s_mutex);
    s_contents[windowId] = slots;
}

bool BlockDataStore::GetBlockActor(const BlockPos& pos,
                                   std::vector<uint8_t>& rawNbt,
                                   std::map<std::string, std::string>& fields)
{
    std::lock_guard<std::mutex> lock(s_mutex);
    auto it = s_blockActors.find(PackPos(pos));
    if (it == s_blockActors.end()) return false;
    rawNbt = it->second.first;
    fields = it->second.second;
    return true;
}

bool BlockDataStore::GetContainerAt(const BlockPos& pos, ContainerOpenInfo& info, std::vector<SlotItem>& slots)
{
    std::lock_guard<std::mutex> lock(s_mutex);
    auto it = s_posToWindow.find(PackPos(pos));
    if (it == s_posToWindow.end()) return false;
    uint8_t windowId = it->second;

    auto oit = s_openInfo.find(windowId);
    if (oit != s_openInfo.end()) info = oit->second;

    auto cit = s_contents.find(windowId);
    if (cit != s_contents.end()) slots = cit->second;

    return true;
}

void BlockDataStore::Clear()
{
    std::lock_guard<std::mutex> lock(s_mutex);
    s_blockActors.clear();
    s_openInfo.clear();
    s_contents.clear();
    s_posToWindow.clear();
}

size_t BlockDataStore::BlockActorCount()
{
    std::lock_guard<std::mutex> lock(s_mutex);
    return s_blockActors.size();
}
