#pragma once
#include <cstdint>
#include <map>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include "CommandBlockUpdate.h"   // BlockPos
#include "ContainerOpen.h"        // ContainerOpenInfo
#include "InventoryContent.h"     // SlotItem

// Thread-safe cache for block data received from the server:
//   - block entity NBT (BlockActorDataPacket, 56) keyed by position
//   - opened container windows (ContainerOpenPacket, 46 / InventoryContentPacket, 49)
//     keyed by window id and also indexed by block position
class BlockDataStore
{
public:
    static void StoreBlockActor(const BlockPos& pos,
                                const std::vector<uint8_t>& rawNbt,
                                const std::map<std::string, std::string>& fields);

    static void StoreContainerOpen(const ContainerOpenInfo& info);
    static void StoreContainerContent(uint8_t windowId, const std::vector<SlotItem>& slots);

    // Returns cached block entity data at pos; false if nothing cached yet.
    static bool GetBlockActor(const BlockPos& pos,
                              std::vector<uint8_t>& rawNbt,
                              std::map<std::string, std::string>& fields);

    // Returns the open-window info and slots for a container block position.
    static bool GetContainerAt(const BlockPos& pos, ContainerOpenInfo& info, std::vector<SlotItem>& slots);

    static void Clear();
    static size_t BlockActorCount();

private:
    static int64_t PackPos(const BlockPos& p);

    static std::mutex s_mutex;

    // packed pos -> (raw nbt, flattened fields)
    static std::unordered_map<int64_t, std::pair<std::vector<uint8_t>, std::map<std::string, std::string>>> s_blockActors;

    // window id -> open info / slots
    static std::unordered_map<uint8_t, ContainerOpenInfo> s_openInfo;
    static std::unordered_map<uint8_t, std::vector<SlotItem>> s_contents;

    // packed pos -> window id
    static std::unordered_map<int64_t, uint8_t> s_posToWindow;
};
