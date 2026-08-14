#pragma once
#include "ConnectInstance.h"
#include "TanGame.h"   // 你的 TanLobbyGameCtx

class NetherNetConnectInstance : public ConnectInstance {
public:
    NetherNetConnectInstance(std::string chain, std::string skindata,
        std::string host_nethernet_id,
        std::string from_nethernet_id,
        std::string md5_token_b64,
        std::string signaling_ip, int signaling_port,
        uint32_t user_id,
        EVP_PKEY* ec_key, ClientInstance* instance);

    ~NetherNetConnectInstance() override;

    void Connect() override;
    void Disconnect() override;
    int WritePacket(PacketBase& Packet) override;
    int WritePacket(std::string Packet) override;
    int WritePacket(std::vector<uint8_t> Packet) override;

private:
    // 在 DataChannel 上收到数据时调
    void onDataChannelMessage(const std::vector<uint8_t>& data);

    // 发数据用的内部方法,跟父类的 SendPacket 一样的加密/压缩处理,但不加 0xFE,不走 RakNet
    void flushSendBuffer();
    void sendThread();

    // NetherNet ctx 持有
    std::shared_ptr<TanLobbyGameCtx> m_nethernet_ctx;

    // NetherNet 连接参数
    std::string m_host_nethernet_id;
    std::string m_from_nethernet_id;
    std::string m_md5_token_b64;
    std::string m_signaling_ip;
    int m_signaling_port;
    uint32_t m_user_id;

    // 收到 DataChannel onOpen 后开始走握手
    std::atomic<bool> m_dc_opened{ false };

    std::thread m_send_thread_nn;
};