#pragma once

#include "PlayerList.h"
#include "StartGame.h"
#include <mutex>
#include <vector>

struct WorldStateSnapshot {
    bool available = false;
    int64_t playerEntityID = 0;
    int64_t entityRuntimeID = 0;
    int gamemode = 0;
    Vec3 position{};
};

class EntityStateStore {
public:
    static void StorePlayers(const PlayerListPacket& packet);
    static void StoreWorld(const StartGame& packet);
    static std::vector<PlayerListEntry> GetPlayers();
    static WorldStateSnapshot GetWorld();
    static void Clear();

private:
    static std::mutex s_mutex;
    static std::vector<PlayerListEntry> s_players;
    static WorldStateSnapshot s_world;
};
