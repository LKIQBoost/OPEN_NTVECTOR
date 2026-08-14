#pragma once
#include <functional>
#include <thread>
#include <memory>
#include "LoginSession.h"
#include "ConnectInstance.h"
#include "ConfigLoader.h"
#include "CallbackManager.h"
#include "LocalPlayer.h"
#include "NetherNetConnectInstance.h"

class ClientInstance
{
public:
    ClientInstance() = default;   // 用类内初始化 + LocalPlayer 默认构造,不需要 memset
    ~ClientInstance() {
        m_is_disconnect = true;   // 通知 BaseTick 退出
        if (m_baseTick.joinable()) {
            m_baseTick.join();    // 等线程退完
        }
        // m_connection / m_localPlayer / m_session_info 由 unique_ptr 自动释放
    }
    // === RakNet 路径 ===
    // session: 已完成登录的产物,所有权移交进来
    void startUp(LoginSession session, std::string server_ip, int server_port) {
        if (Params::logger)
            std::cout << session.chain << std::endl;
        if (!session.valid()) {
            LOG(LOG_ERROR, "[ClientInstance] startUp - Invalid login session");
            return;
        }
        m_session_info = std::move(session);

        LOG(LOG_NETWORK, "[ClientInstance] Creating RakNet connection to ",
            server_ip, ":", server_port);

        m_connection = std::make_unique<ConnectInstance>(
            m_session_info.chain,
            m_session_info.skinJwt,
            server_ip, server_port,
            m_session_info.getECKey(),   // 借用,LoginSession 仍是所有者
            this);

        SocketInitialize();

        if (Params::disout) {
            m_connection->setDisconnectExit();
        }
        m_connection->Connect();
    }

    // === NetherNet 路径(暂时保留旧逻辑,回头再迁) ===
    void startUp_NetherNet(LoginSession session,
        std::string host_nethernet_id,
        std::string from_nethernet_id,
        std::string md5_token_b64,
        std::string signaling_ip,
        int signaling_port,
        uint32_t user_id)
    {
        if (!session.valid()) {
            LOG(LOG_ERROR, "[ClientInstance] startUp_NetherNet - Invalid login session");
            return;
        }
        if(Params::logger)
            std::cout << session.chain << std::endl;
        m_session_info = std::move(session);

        LOG(LOG_NETWORK, "[ClientInstance] Creating NetherNet connection: signaling=",
            signaling_ip, ":", signaling_port,
            ", host_nid=", host_nethernet_id);

        m_connection = std::make_unique<NetherNetConnectInstance>(
            m_session_info.chain,
            m_session_info.skinJwt,
            host_nethernet_id, from_nethernet_id, md5_token_b64,
            signaling_ip, signaling_port, user_id,
            m_session_info.getECKey(),
            this);

        SocketInitialize();

        if (Params::disout) {
            m_connection->setDisconnectExit();
        }
        m_connection->Connect();
    }

    void disconnect() {
        m_is_disconnect = true;
        if (m_connection) {
            m_connection->Disconnect();
            m_connection.reset();
        }
        if (m_baseTick.joinable()) {
            m_baseTick.join();
        }
    }

    void BaseTick();
    void StartTick();
    void SetTickHandle(std::function<void(ClientInstance*)>);

    LocalPlayer* getLocalPlayer() { return m_localPlayer.get(); }
    ConnectInstance* getInstance() { return m_connection.get(); }
    EVP_PKEY* getECKey()       const { return m_session_info.getECKey(); }

    unsigned __int64 getTick() const { return Tick; }

    void SetSourceAuthInput(bool source) { m_source_auth_input = source; }
    bool GetSourceAuthInput() const { return m_source_auth_input; }

    void respawn() {
        Respawn res{};
        res.Position = Vec3();
        res.State = 2;
        res.EntityRuntimeID = getLocalPlayer()->EntityRuntimeID;
        getInstance()->WritePacket(res);

        PlayerAction pa{};
        pa.EntityRuntimeID = getLocalPlayer()->EntityRuntimeID;
        pa.ActionType = 0x0E;
        pa.BlockFace = 1;
        getInstance()->WritePacket(pa);

        std::vector<uint8_t> abcd = { 0xb8,0x02,0x04,0x00 };
        getInstance()->WritePacket(abcd);
    }

    void updateLocalPlayerStateOnServer() {
        if (!m_source_auth_input) {
            if (TickHandel != nullptr) {
                TickHandel(this);
            }
        }
    }

private:
    void SocketInitialize() {
        LOG(LOG_INFO, "[ClientInstance] SocketInitialize - Registering packet callbacks");
        m_connection->RegisterReceiveCallBack(IDNetworkSettings, CallbackManager::onNetworkSetting);
        m_connection->RegisterReceiveCallBack(IDServerToClientHandshake, CallbackManager::onServerToClientHandshake);
        m_connection->RegisterReceiveCallBack(IDPlayStatus, CallbackManager::onPlayStatus);
        m_connection->RegisterReceiveCallBack(IDResourcePacksInfo, CallbackManager::onResourcePacksInfo);
        m_connection->RegisterReceiveCallBack(IDDisconnect, CallbackManager::onDisconnect);
        m_connection->RegisterReceiveCallBack(IDStartGame, CallbackManager::onStartGame);
        m_connection->RegisterReceiveCallBack(IDPyRpc, CallbackManager::onPyRpc);
        m_connection->RegisterReceiveCallBack(IDContainerOpen, CallbackManager::onContainerOpen);
        m_connection->RegisterReceiveCallBack(IDInventoryContent, CallbackManager::onInventoryContent);
        m_connection->RegisterReceiveCallBack(IDBlockActorData, CallbackManager::onBlockActorData);
        m_connection->RegisterReceiveCallBack(IDSubChunk, CallbackManager::onSubChunk);
        //m_connection->RegisterReceiveCallBack(IDText, CallbackManager::onText);
        //m_connection->RegisterReceiveCallBack(IDAddPlayer, CallbackManager::onAddPlayer);
        //m_connection->RegisterReceiveCallBack(IDPlayerList, CallbackManager::onPlayerList);
        //m_connection->RegisterReceiveCallBack(IDDeathInfo, CallbackManager::onDeathInfo);
        //m_connection->RegisterReceiveCallBack(IDCommandOutput, CallbackManager::onIDCommandOutput);
        m_connection->RegisterReceiveCallBack(IDMovePlayer, CallbackManager::onIDMovePlayer);
        //m_connection->RegisterReceiveCallBack(IDMoveActorAbsolute, CallbackManager::onIDMoveActorAbsolute);
        m_connection->RegisterReceiveCallBack(IDCorrectPlayerMovePrediction, CallbackManager::onIDCorrectPlayerMovePrediction);
        //m_connection->RegisterReceiveCallBack(IDTickSync, CallbackManager::onIDTickSync);
        //m_connection->RegisterReceiveCallBack(IDUpdatePlayerGameType, CallbackManager::onIDUpdatePlayerGameType);
        //m_connection->RegisterReceiveCallBack(IDSetHealth, CallbackManager::onIDSetHealth);
        //m_connection->RegisterReceiveCallBack(IDRespawn, CallbackManager::onIDRespawn);
        LOG(LOG_INFO, "[ClientInstance] SocketInitialize - All packet callbacks registered");
    }

    // ★ 声明顺序重要 - LoginSession 最先声明 → 最后析构(活最久)
    //                    m_connection 在它之后 → 早一步析构(它借用 m_session_info 的 ecKey)
    LoginSession                     m_session_info;
    std::unique_ptr<ConnectInstance> m_connection;
    std::unique_ptr<LocalPlayer>     m_localPlayer = std::make_unique<LocalPlayer>();

    bool             m_is_disconnect = false;
    bool             m_source_auth_input = false;
    unsigned __int64 Tick = 0;
    std::function<void(ClientInstance*)> TickHandel;
    std::thread      m_baseTick;
};