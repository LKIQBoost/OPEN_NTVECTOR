#include "StartGame.h"
#include "Logger.h"

unsigned char StartGame::ID()
{
    return IDStartGame;
}

void StartGame::Deserializ(std::vector<unsigned char> pack)
{
    LOG(LOG_SERVER, "[StartGame] Deserializing start game packet, size: ", pack.size());
    BinaryReader br(pack.data(), pack.size());
    playerEntityID = br.ReadVarInt64();
    EntityRuntimeID = br.ReadVarInt64();
	gamemode = br.ReadVarInt();
	PlayerPosition = br.ReadVec3();
	Pitch = br.ReadFloat();
	Yaw = br.ReadFloat();
    LOG(LOG_INFO, "[StartGame] Player Entity ID: ", playerEntityID);
}
