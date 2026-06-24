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
    using OnDataChannelMessage = std::function<void(const std::vector<uint8_t>&)>;
    using OnDataChannelOpen = std::function<void()>;
    using OnDataChannelClose = std::function<void()>;

    explicit TanLobbyGameCtx(bool disout = false);
    ~TanLobbyGameCtx();
    TanLobbyGameCtx(const TanLobbyGameCtx&) = delete;
    TanLobbyGameCtx& operator=(const TanLobbyGameCtx&) = delete;

    void startUp(const std::string& nethernet_id_to,
        const std::string& nethernet_id_from,
        const std::string& token,
        const std::string& server_ip,
        int server_port,
        uint32_t user_id);

    void shutdown();

    void setOnDataChannelMessage(OnDataChannelMessage cb);
    void setOnDataChannelOpen(OnDataChannelOpen cb);
    void setOnDataChannelClose(OnDataChannelClose cb);

    bool sendDataChannelMessage(const std::vector<uint8_t>& data);

    void onDataReceived(const std::string& content, size_t size);
    void onConnection(bool connected);

    static std::shared_ptr<TanLobbyGameCtx> create(bool disout = false);
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

    void startWsConnectTimeout();
    void startTurnConfigTimeout();
    void startConnectResponseTimeout();
    void startWebRtcTimeout();
    void handleConnectionLost();
    void doExit(const char* event_name, int exit_code);

    bool m_disout = false;

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

    std::atomic<bool> m_dc_open{ false };
    std::mutex m_callback_mutex;
    OnDataChannelMessage m_on_dc_message;
    OnDataChannelOpen    m_on_dc_open;
    OnDataChannelClose   m_on_dc_close;

    bool m_initialized = false;
    bool m_running = false;

    // timeout constants
    static constexpr int WS_CONNECT_TIMEOUT_MS = 10000;
    static constexpr int TURN_CONFIG_TIMEOUT_MS = 10000;
    static constexpr int CONNECT_RESPONSE_TIMEOUT_MS = 5000;
    static constexpr int WEBRTC_CONNECT_TIMEOUT_MS = 15000;

    // exit codes
    static constexpr int EXIT_CODE_WS_CONNECT = 2;
    static constexpr int EXIT_CODE_WS_TIMEOUT = 3;
    static constexpr int EXIT_CODE_NetherNet_TIMEOUT = 4;
    static constexpr int EXIT_CODE_WS_Invalid_token = 5;
    static constexpr int EXIT_CODE_TURN_TIMEOUT = 6;

    // timeout state
    std::atomic<bool> m_ws_connected{ false };
    std::atomic<bool> m_connect_response_received{ false };
    std::atomic<bool> m_pc_connected{ false };
    std::atomic<bool> m_closing{ false };
    std::atomic<bool> m_connection_lost_reported{ false };

    std::thread m_ws_timeout_thread;
    std::thread m_turn_timeout_thread;
    std::thread m_connect_timeout_thread;
    std::thread m_webrtc_timeout_thread;
};

void register_tan_lobby_game_module();
