// TanGame.h
#pragma once
#include <pybind11/pybind11.h>
#include <memory>
#include <string>
#include <cstdint>
#include <atomic>
#include <thread>
#include <mutex>
#include <vector>
#include <functional>

namespace py = pybind11;
class WebSocketClient;
namespace rtc {
    class PeerConnection;
    class DataChannel;
}

class TanLobbyGameCtx : public std::enable_shared_from_this<TanLobbyGameCtx> {
public:
    // 回调类型
    using OnDataChannelMessage = std::function<void(const std::vector<uint8_t>&)>;
    using OnDataChannelOpen = std::function<void()>;
    using OnDataChannelClose = std::function<void()>;

    TanLobbyGameCtx();
    ~TanLobbyGameCtx();
    TanLobbyGameCtx(const TanLobbyGameCtx&) = delete;
    TanLobbyGameCtx& operator=(const TanLobbyGameCtx&) = delete;

    void startUp(const std::string& nethernet_id_to,
        const std::string& nethernet_id_from,
        const std::string& token,
        const std::string& server_ip,
        int server_port,
        uint32_t user_id);

    // ★ 新增:主动关闭
    void shutdown();

    // ★ 新增:注册 DataChannel 回调
    //   注意:必须在 startUp() 之前调,否则 onOpen/onMessage 可能已经触发了
    void setOnDataChannelMessage(OnDataChannelMessage cb);
    void setOnDataChannelOpen(OnDataChannelOpen cb);
    void setOnDataChannelClose(OnDataChannelClose cb);

    // ★ 新增:通过 DataChannel 发数据
    //   线程安全,失败返回 false(DC 未 open 或已关闭)
    bool sendDataChannelMessage(const std::vector<uint8_t>& data);

    // WebSocket 回调(已存在,保持不变)
    void onDataReceived(const std::string& content, size_t size);
    void onConnection(bool connected);

    static std::shared_ptr<TanLobbyGameCtx> create();
    static int64_t GetRandomData();

private:
    void sendControlMessage(int type);
    void sendSignalingMessage(const std::string& message_payload);
    void startKeepalive();
    void stopKeepalive();
    void handleIncomingMessage(const std::string& json_str);
    void handleType2_TurnConfig(const std::string& turn_config_json);
    void handleType1_Signaling(const std::string& from, const std::string& message);
    void setupPeerConnection(const std::string& turn_config_json);

    std::atomic<bool> m_received_any{ false };
    std::atomic<bool> m_keepalive_running{ false };
    std::atomic<bool> m_turn_received{ false };
    std::thread m_keepalive_thread;

    std::string m_nethernet_id_to;
    std::string m_nethernet_id_from;
    std::string m_token;
    std::string m_server_ip;
    int         m_server_port = 0;
    uint32_t    m_user_id = 0;

    std::unique_ptr<WebSocketClient> m_ws_client;
    std::shared_ptr<rtc::PeerConnection> m_pc;
    std::shared_ptr<rtc::DataChannel> m_dc_reliable;
    std::shared_ptr<rtc::DataChannel> m_dc_unreliable;
    std::string m_connection_id;
    std::mutex  m_signaling_mutex;

    // ★ 新增:DC 状态 + 用户回调
    std::atomic<bool> m_dc_open{ false };
    std::mutex m_callback_mutex;
    OnDataChannelMessage m_on_dc_message;
    OnDataChannelOpen    m_on_dc_open;
    OnDataChannelClose   m_on_dc_close;

    bool m_initialized = false;
    bool m_running = false;

    // 超时检测相关常量(暂时硬编码,以后可改)
    static constexpr int CONNECT_RESPONSE_TIMEOUT_MS = 5000;

    // 常量
    static constexpr int EXIT_CODE_NetherNet_TIMEOUT = 4;     // WebSocket/信令超时统一退出码 3
    static constexpr int EXIT_CODE_WS_TIMEOUT = 3;     // WebSocket/信令超时统一退出码 3
    static constexpr int EXIT_CODE_WS_Invalid_token = 5;     // WebSocket/信令超时统一退出码 3

    // 私有字段
    std::atomic<bool> m_connect_response_received{ false };
    std::thread       m_connect_timeout_thread;
    // 常量
    static constexpr int WEBRTC_CONNECT_TIMEOUT_MS = 15000;   // ICE+DTLS+SCTP 全程超时,给 15s

    // 字段
    std::atomic<bool> m_pc_connected{ false };       // PC 是否到过 Connected
    std::atomic<bool> m_closing{ false };            // 是否走正常 shutdown,避免误报
    std::thread m_webrtc_timeout_thread;

    // 方法声明
    void startWebRtcTimeout();
    void handleConnectionLost();   // 统一的 close→out 处理
    // 私有方法声明
    void startConnectResponseTimeout();
};

void register_tan_lobby_game_module();