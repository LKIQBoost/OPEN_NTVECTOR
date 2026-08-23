#include "../WebSocketClient.h"
#include <libwebsockets.h>
#include <thread>
#include <atomic>
#include <mutex>
#include <vector>
#include <queue>
#include <string>
#include <cstring>

static int ws_callback(struct lws* wsi, enum lws_callback_reasons reason,
                       void* user, void* in, size_t len);

static const struct lws_protocols ws_protocols[] = {
    {"ws-protocol", ws_callback, 0, 65536},
    LWS_PROTOCOL_LIST_TERM
};

class WebSocketClientImpl : public IWebSocketClient {
public:
    WebSocketClientImpl(const std::string& serverIp, int serverPort,
                        const std::string& protocol, const std::string& path)
        : m_serverIp(serverIp), m_serverPort(serverPort),
          m_protocol(protocol), m_path(path) {}

    ~WebSocketClientImpl() override { disconnect(); }

    bool connect(OnDataReceived onDataCb, OnConnectionState onConnCb) override {
        m_onData = std::move(onDataCb);
        m_onConn = std::move(onConnCb);
        m_running = true;

        struct lws_context_creation_info ctxInfo{};
        ctxInfo.port = CONTEXT_PORT_NO_LISTEN;
        ctxInfo.protocols = ws_protocols;
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
        connInfo.local_protocol_name = ws_protocols[0].name;
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
        {
            std::lock_guard<std::mutex> lock(m_sendMutex);
            m_sendQueue.push(data);
        }
        lws_cancel_service(m_context);
        return true;
    }

    bool isConnected() const override { return m_connected; }

    // --- 以下由 ws_callback 在 lws service 线程调用 ---

    void onWsEstablished() {
        m_connected = true;
        if (m_onConn) m_onConn(true);
    }

    void onWsReceive(void* in, size_t len) {
        if (!in || !len) return;
        std::lock_guard<std::mutex> lock(m_recvMutex);
        m_recvBuf.insert(m_recvBuf.end(),
            static_cast<char*>(in), static_cast<char*>(in) + len);
    }

    void onWsReceiveComplete(struct lws* wsi) {
        size_t remaining = lws_remaining_packet_payload(wsi);
        bool isFinal = lws_is_final_fragment(wsi);
        if (remaining == 0 && isFinal) {
            std::string message;
            {
                std::lock_guard<std::mutex> lock(m_recvMutex);
                message.swap(m_recvBuf);
            }
            if (m_onData) {
                m_onData(message, message.size());
            }
        }
    }

    void onWsClosed() {
        m_connected = false;
        m_recvBuf.clear();
        if (m_onConn) m_onConn(false);
    }

    bool hasPendingData() {
        std::lock_guard<std::mutex> lock(m_sendMutex);
        return !m_sendQueue.empty();
    }

    struct lws* getWsi() const { return m_wsi; }

    void onWsWritable(struct lws* wsi) {
        std::lock_guard<std::mutex> lock(m_sendMutex);
        while (!m_sendQueue.empty()) {
            const std::string& data = m_sendQueue.front();
            std::vector<unsigned char> buf(LWS_PRE + data.size());
            std::memcpy(buf.data() + LWS_PRE, data.data(), data.size());
            lws_write(wsi, buf.data() + LWS_PRE, data.size(), LWS_WRITE_TEXT);
            m_sendQueue.pop();
        }
    }

private:
    std::string m_serverIp, m_protocol, m_path;
    int m_serverPort = 0;
    struct lws_context* m_context = nullptr;
    struct lws* m_wsi = nullptr;
    std::thread m_thread;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_connected{false};
    std::mutex m_sendMutex;
    std::mutex m_recvMutex;
    std::string m_recvBuf;
    std::queue<std::string> m_sendQueue;
    OnDataReceived m_onData;
    OnConnectionState m_onConn;
};

static int ws_callback(struct lws* wsi, enum lws_callback_reasons reason,
                       void* user, void* in, size_t len) {
    struct lws_context* ctx = lws_get_context(wsi);
    auto* self = static_cast<WebSocketClientImpl*>(lws_context_user(ctx));
    if (!self) return 0;

    switch (reason) {
    case LWS_CALLBACK_CLIENT_ESTABLISHED:
        self->onWsEstablished();
        if (self->hasPendingData())
            lws_callback_on_writable(wsi);
        break;

    case LWS_CALLBACK_CLIENT_RECEIVE:
        self->onWsReceive(in, len);
        self->onWsReceiveComplete(wsi);
        break;

    case LWS_CALLBACK_CLIENT_WRITEABLE:
        self->onWsWritable(wsi);
        if (self->hasPendingData())
            lws_callback_on_writable(wsi);
        break;

    case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
    case LWS_CALLBACK_CLOSED:
        self->onWsClosed();
        break;

    case LWS_CALLBACK_EVENT_WAIT_CANCELLED:
        if (self->getWsi())
            lws_callback_on_writable(self->getWsi());
        break;

    default:
        break;
    }
    return 0;
}

std::unique_ptr<IWebSocketClient> IWebSocketClient::Create(
    const std::string& serverIp, int serverPort,
    const std::string& protocol, const std::string& path) {
    return std::make_unique<WebSocketClientImpl>(serverIp, serverPort, protocol, path);
}
