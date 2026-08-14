#pragma once
#include <vector>
#include "ConnectInstance.h"
#include "Logger.h"
#include "EasyUtils.cpp"
#include "Base64Cpp.h"
#include "json/json.h"
#include "ECDSA.h"
#include "StartupParams.h"
#include "engine_wrapper.h"


class CallbackManager
{
public:
    static void onNetworkSetting(ConnectInstance*, std::vector<uint8_t>);
    static void onServerToClientHandshake(ConnectInstance*, std::vector<uint8_t>);
    static void onPlayStatus(ConnectInstance*, std::vector<uint8_t>);
    static void onResourcePacksInfo(ConnectInstance*, std::vector<uint8_t>);
    static void onStartGame(ConnectInstance*, std::vector<uint8_t>);
    static void onStartGameEvent(ConnectInstance*, std::vector<uint8_t>);
    static void onPyRpc(ConnectInstance*, std::vector<uint8_t>);
    static void onDisconnect(ConnectInstance*, std::vector<uint8_t>);
    static void onDisconnectEvent(ConnectInstance*, std::vector<uint8_t>);
    static void onText(ConnectInstance*, std::vector<uint8_t>);
    static void onAddPlayer(ConnectInstance*, std::vector<uint8_t>);
    static void onPlayerList(ConnectInstance*, std::vector<uint8_t>);
    static void onDeathInfo(ConnectInstance*, std::vector<uint8_t>);
    static void onIDCommandOutput(ConnectInstance*, std::vector<uint8_t>);
    static void onIDMovePlayer(ConnectInstance*, std::vector<uint8_t>);
    static void onIDMoveActorAbsolute(ConnectInstance*, std::vector<uint8_t>);
    static void onIDCorrectPlayerMovePrediction(ConnectInstance*, std::vector<uint8_t>);
    static void onIDTickSync(ConnectInstance*, std::vector<uint8_t>);
    static void onIDUpdatePlayerGameType(ConnectInstance*, std::vector<uint8_t>);
    static void onIDSetHealth(ConnectInstance*, std::vector<uint8_t>);
    static void onIDRespawn(ConnectInstance*, std::vector<uint8_t>);


    static void onContainerOpen(ConnectInstance*, std::vector<uint8_t>);
    static void onInventoryContent(ConnectInstance*, std::vector<uint8_t>);
    static void onBlockActorData(ConnectInstance*, std::vector<uint8_t>);
    static void onSubChunk(ConnectInstance*, std::vector<uint8_t>);
    static void runLoopAuthInput(ClientInstance*);
};