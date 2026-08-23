#include "EntityStateStore.h"
#include <algorithm>
#include <algorithm>

std::mutex EntityStateStore::s_mutex;
std::vector<PlayerListEntry> EntityStateStore::s_players;
WorldStateSnapshot EntityStateStore::s_world;

void EntityStateStore::StorePlayers(const PlayerListPacket& packet)
{
    std::lock_guard<std::mutex> lock(s_mutex);
    if (packet.IsAddAction()) {
        for (const auto& incoming : packet.Entries) {
            auto it = std::find_if(s_players.begin(), s_players.end(), [&](const auto& current) {
                return current.UUID == incoming.UUID;
            });
            if (it == s_players.end()) s_players.push_back(incoming);
            else *it = incoming;
        }
    } else {
        for (const auto& removed : packet.Entries) {
            s_players.erase(std::remove_if(s_players.begin(), s_players.end(), [&](const auto& current) {
                return current.UUID == removed.UUID;
            }), s_players.end());
        }
    }
}

void EntityStateStore::StoreWorld(const StartGame& packet)
{
    std::lock_guard<std::mutex> lock(s_mutex);
    s_world.available = true;
    s_world.playerEntityID = packet.playerEntityID;
    s_world.entityRuntimeID = packet.EntityRuntimeID;
    s_world.gamemode = packet.gamemode;
    s_world.position = packet.PlayerPosition;
}

std::vector<PlayerListEntry> EntityStateStore::GetPlayers()
{
    std::lock_guard<std::mutex> lock(s_mutex);
    return s_players;
}

WorldStateSnapshot EntityStateStore::GetWorld()
{
    std::lock_guard<std::mutex> lock(s_mutex);
    return s_world;
}

void EntityStateStore::Clear()
{
    std::lock_guard<std::mutex> lock(s_mutex);
    s_players.clear();
    s_world = {};
}
