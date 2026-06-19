#include "NetherNetConnectInstance.h"

extern unsigned int MinecraftBedrockProtocolVersion;
NetherNetConnectInstance::NetherNetConnectInstance(
    std::string chain, std::string skindata,
    std::string host_nethernet_id, std::string from_nethernet_id,
    std::string md5_token_b64, std::string signaling_ip, int signaling_port,
    uint32_t user_id, EVP_PKEY* ec_key, ClientInstance* instance)
    // 父类 ctor: 用空 ip/port,因为这条路径不用普通 ip/port
    : ConnectInstance(std::move(chain), std::move(skindata), "", 0, ec_key, instance),
    m_host_nethernet_id(std::move(host_nethernet_id)),
    m_from_nethernet_id(std::move(from_nethernet_id)),
    m_md5_token_b64(std::move(md5_token_b64)),
    m_signaling_ip(std::move(signaling_ip)),
    m_signaling_port(signaling_port),
    m_user_id(user_id)
{
}

NetherNetConnectInstance::~NetherNetConnectInstance() {
    Disconnect();
    if (m_send_thread_nn.joinable()) m_send_thread_nn.join();
}

void NetherNetConnectInstance::Connect() {
    if (!m_is_disconnect) return;
    m_is_disconnect = false;

    m_nethernet_ctx = TanLobbyGameCtx::create();

    // 注入回调: DC 收到包 -> 喂给父类 HandlePakcet
    m_nethernet_ctx->setOnDataChannelMessage([this](const std::vector<uint8_t>& data) {
        this->onDataChannelMessage(data);
        });

    m_nethernet_ctx->setOnDataChannelOpen([this]() {
        LOG(LOG_NETWORK, "[NetherNet] DataChannel opened, ready to send");
        m_dc_opened = true;

        // 跟 RakNet 路径上 ID_CONNECTION_REQUEST_ACCEPTED 一样的初始化
        RequestNetworkSettings r;
        r.ProtocolVersion = MinecraftBedrockProtocolVersion;
        WritePacket(r);
        });

    m_nethernet_ctx->startUp(
        m_host_nethernet_id, m_from_nethernet_id,
        m_md5_token_b64, m_signaling_ip, m_signaling_port, m_user_id);

    // 起发送线程
    m_send_thread_nn = std::thread(&NetherNetConnectInstance::sendThread, this);
    m_send_thread_nn.detach();
}

void NetherNetConnectInstance::Disconnect() {
    m_is_disconnect = true;
    if (m_nethernet_ctx) {
        m_nethernet_ctx->shutdown();   // 你 TanLobbyGameCtx 上需要有这个方法
        m_nethernet_ctx.reset();
    }
}

int NetherNetConnectInstance::WritePacket(PacketBase& Packet) {
    std::vector<uint8_t> data = Packet.Serializ();
    m_buffer.appendData(data);
    return 0;
}

int NetherNetConnectInstance::WritePacket(std::string Packet) {
    m_buffer.appendData(std::vector<uint8_t>(Packet.data(), Packet.data() + Packet.size()));
    return 0;
}

int NetherNetConnectInstance::WritePacket(std::vector<uint8_t> Packet) {
    m_buffer.appendData(Packet);
    return 0;
}

void NetherNetConnectInstance::sendThread() {
    while (!m_is_disconnect) {
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
        if (!m_dc_opened) continue;
        if (!m_buffer.hasDataInCurrentBuffer()) continue;
        BufferSlice Buffer = m_buffer.swapAndGetBuffer();
        std::vector<uint8_t> data(Buffer.data, Buffer.data + Buffer.size);

        if (is_networksetting)
            data = m_algorithm->Compress(data);

        if (is_servertoclient) {
            unsigned char hash[32];
            unsigned char ulong[8];
            memcpy(ulong, &m_sendnum, 8);
            std::vector<uint8_t> buf(ulong, ulong + 8);
            buf.insert(buf.end(), data.begin(), data.end());
            buf.insert(buf.end(), SESSING_KEY.begin(), SESSING_KEY.end());
            calculate_sha256((char*)buf.data(), buf.size(), hash);
            m_sendnum++;
            data.insert(data.end(), hash, hash + 8);
            SendKey->encrypt(data);
        }

        // ★ 网易 NetherNet 子协议头 0x00
        std::vector<uint8_t> framed;
        framed.reserve(1 + data.size());
        framed.push_back(0x00);
        framed.insert(framed.end(), data.begin(), data.end());
        m_nethernet_ctx->sendDataChannelMessage(framed);

        m_buffer.finishSending();
    }
}

void NetherNetConnectInstance::onDataChannelMessage(const std::vector<uint8_t>& data) {
    if (data.empty()) return;

    // ★ 网易 NetherNet 子协议头(应该是 0x00),剥掉
    // 防御性检查:如果第一字节不是 0x00,记一下日志
    if (data[0] != 0x00) {
        LOG(LOG_NETWORK, "[NetherNet] unexpected header byte: 0x",
            std::hex, (int)data[0], std::dec, " (expected 0x00)");
    }

    if (data.size() < 1) return;
    HandlePakcet(const_cast<uint8_t*>(data.data()) + 1, data.size() - 1);
}