#ifndef _WIN32
#include "../WebSocketClient.h"
#include <libwebsockets.h>
#include <thread>
#include <atomic>
#include <mutex>
#include <string>
#include <cstring>

class LinuxWebSocketClient : public IWebSocketClient {
public:
    LinuxWebSocketClient(const std::string& serverIp, int serverPort,
                         const std::string& protocol, const std::string& path)
        : m_serverIp(serverIp), m_serverPort(serverPort),
          m_protocol(protocol), m_path(path) {}

    ~LinuxWebSocketClient() override { disconnect(); }

    bool connect(OnDataReceived onDataCb, OnConnectionState onConnCb) override {
        m_onData = std::move(onDataCb);
        m_onConn = std::move(onConnCb);
        m_running = true;

        struct lws_context_creation_info ctxInfo{};
        ctxInfo.port = CONTEXT_PORT_NO_LISTEN;
        ctxInfo.protocols = s_protocols;
        ctxInfo.user = this;

        m_context = lws_create_context(&ctxInfo);
        if (!m_context) return false;

        struct lws_client_connect_info connInfo{};
        connInfo.context = m_context;
        connInfo.address = m_serverIp.c_str();
        connInfo.port = m_serverPort;
        connInfo.path = m_path.c_str();
        connInfo.host = m_serverIp.c_str();
        connInfo.protocol = m_protocol.c_str();
        connInfo.userdata = this;

        m_wsi = lws_client_connect_via_info(&connInfo);
        if (!m_wsi) {
            lws_context_destroy(m_context);
            m_context = nullptr;
            return false;
        }

        m_thread = std::thread([this]() {
            while (m_running && m_context) {
                lws_service(m_context, 50);
            }
        });

        return true;
    }

    void disconnect() override {
        m_running = false;
        if (m_thread.joinable()) m_thread.join();
        if (m_context) {
            lws_context_destroy(m_context);
            m_context = nullptr;
        }
        m_wsi = nullptr;
        m_connected = false;
    }

    bool sendData(const std::string& data) override {
        if (!m_connected || !m_wsi) return false;
        std::lock_guard<std::mutex> lock(m_mutex);
        std::vector<unsigned char> buf(LWS_PRE + data.size());
        std::memcpy(buf.data() + LWS_PRE, data.data(), data.size());
        int n = lws_write(m_wsi, buf.data() + LWS_PRE, data.size(), LWS_WRITE_BINARY);
        return n >= 0;
    }

    bool isConnected() const override { return m_connected; }

private:
    static int callback(struct lws* wsi, enum lws_callback_reasons reason,
                        void* user, void* in, size_t len) {
        auto* self = static_cast<LinuxWebSocketClient*>(
            lws_context_user(lws_get_context(wsi)));
        if (!self) return 0;

        switch (reason) {
        case LWS_CALLBACK_CLIENT_ESTABLISHED:
            self->m_connected = true;
            if (self->m_onConn) self->m_onConn(true);
            break;
        case LWS_CALLBACK_CLIENT_RECEIVE:
            if (self->m_onData && in) {
                self->m_onData(std::string(static_cast<char*>(in), len), len);
            }
            break;
        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
        case LWS_CALLBACK_CLOSED:
            self->m_connected = false;
            if (self->m_onConn) self->m_onConn(false);
            break;
        default:
            break;
        }
        return 0;
    }

    static const struct lws_protocols s_protocols[];

    std::string m_serverIp, m_protocol, m_path;
    int m_serverPort = 0;
    struct lws_context* m_context = nullptr;
    struct lws* m_wsi = nullptr;
    std::thread m_thread;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_connected{false};
    std::mutex m_mutex;
    OnDataReceived m_onData;
    OnConnectionState m_onConn;
};

const struct lws_protocols LinuxWebSocketClient::s_protocols[] = {
    {"ws-protocol", LinuxWebSocketClient::callback, 0, 65536},
    {nullptr, nullptr, 0, 0}
};

std::unique_ptr<IWebSocketClient> IWebSocketClient::Create(
    const std::string& serverIp, int serverPort,
    const std::string& protocol, const std::string& path) {
    return std::make_unique<LinuxWebSocketClient>(serverIp, serverPort, protocol, path);
}
#endif
