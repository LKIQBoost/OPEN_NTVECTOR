// TanGame.cpp
#include "TanGame.h"
#include "Logger.h"
#include "Base64Cpp.h"
#include "WebSockeVirtualWrapper.h"
#include "engine_wrapper.h"

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <json/json.h>
#include <rtc/rtc.hpp>

#include <regex>
#include <cstring>
#include <cstdio>
#include <cctype>
#include <chrono>
#include <random>
#include <thread>
#include <sstream>
#include <variant>

namespace {

    std::string to_url_safe(std::string b64) {
        for (auto& c : b64) {
            if (c == '+')      c = '-';
            else if (c == '/') c = '_';
        }
        return b64;
    }

    std::string url_encode(const std::string& s) {
        std::string out;
        char buf[8];
        for (unsigned char c : s) {
            if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
                out += static_cast<char>(c);
            }
            else {
                std::snprintf(buf, sizeof(buf), "%%%02X", c);
                out += buf;
            }
        }
        return out;
    }

    void random_bytes(uint8_t* out, size_t n) {
        if (RAND_bytes(out, static_cast<int>(n)) == 1) return;
        static thread_local std::mt19937_64 rng(
            std::random_device{}() ^
            static_cast<uint64_t>(std::chrono::high_resolution_clock::now().time_since_epoch().count()) ^
            static_cast<uint64_t>(std::hash<std::thread::id>{}(std::this_thread::get_id()))
        );
        for (size_t i = 0; i < n; ++i) out[i] = static_cast<uint8_t>(rng() & 0xFF);
    }

    bool aes_ecb_128_encrypt_block(const uint8_t key[16],
        const uint8_t plaintext[16],
        uint8_t ciphertext[16]) {
        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        if (!ctx) return false;
        bool ok = false;
        do {
            if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_128_ecb(), nullptr, key, nullptr)) break;
            EVP_CIPHER_CTX_set_padding(ctx, 0);
            int outlen1 = 0, outlen2 = 0;
            if (1 != EVP_EncryptUpdate(ctx, ciphertext, &outlen1, plaintext, 16)) break;
            if (1 != EVP_EncryptFinal_ex(ctx, ciphertext + outlen1, &outlen2)) break;
            if (outlen1 + outlen2 != 16) break;
            ok = true;
        } while (false);
        EVP_CIPHER_CTX_free(ctx);
        return ok;
    }

    bool parse_json(const std::string& s, Json::Value& out) {
        Json::CharReaderBuilder b;
        std::string err;
        std::istringstream iss(s);
        return Json::parseFromStream(b, iss, &out, &err);
    }

    const char* pc_state_name(rtc::PeerConnection::State s) {
        switch (s) {
        case rtc::PeerConnection::State::New:          return "New";
        case rtc::PeerConnection::State::Connecting:   return "Connecting";
        case rtc::PeerConnection::State::Connected:    return "Connected";
        case rtc::PeerConnection::State::Disconnected: return "Disconnected";
        case rtc::PeerConnection::State::Failed:       return "Failed";
        case rtc::PeerConnection::State::Closed:       return "Closed";
        }
        return "?";
    }

} // namespace


TanLobbyGameCtx::TanLobbyGameCtx(bool disout) : m_disout(disout) {
    LOG(LOG_SCRIPTING, "[TanLobbyGameCtx] Created (disout=", disout, ")");
}

TanLobbyGameCtx::~TanLobbyGameCtx() {
    LOG(LOG_SCRIPTING, "[TanLobbyGameCtx] destructor called");
    shutdown();

    if (m_keepalive_thread.joinable()) {
        m_keepalive_thread.join();
    }
    LOG(LOG_SCRIPTING, "[TanLobbyGameCtx] Destroyed");
}

int64_t TanLobbyGameCtx::GetRandomData() {
    uint8_t buf[8];
    random_bytes(buf, 8);
    int64_t num = 0;
    std::memcpy(&num, buf, 8);
    int64_t result = num % 9223372036854775806LL;
    return result < 0 ? -result : result;
}
void TanLobbyGameCtx::setOnDataChannelMessage(OnDataChannelMessage cb) {
    std::lock_guard<std::mutex> lk(m_callback_mutex);
    m_on_dc_message = std::move(cb);
}

void TanLobbyGameCtx::setOnDataChannelOpen(OnDataChannelOpen cb) {
    std::lock_guard<std::mutex> lk(m_callback_mutex);
    m_on_dc_open = std::move(cb);
    if (m_dc_open && m_on_dc_open) {
        auto local_cb = m_on_dc_open;
        try { local_cb(); }
        catch (const std::exception& e) {
            LOG(LOG_SCRIPTING, "[TanLobbyGameCtx] onOpen callback threw: ", e.what());
        }
    }
}

void TanLobbyGameCtx::setOnDataChannelClose(OnDataChannelClose cb) {
    std::lock_guard<std::mutex> lk(m_callback_mutex);
    m_on_dc_close = std::move(cb);
}
bool TanLobbyGameCtx::sendDataChannelMessage(const std::vector<uint8_t>& data) {
    if (!m_dc_open) {
        LOG(LOG_SCRIPTING, "[TanLobbyGameCtx] sendDataChannelMessage: reliable DC not open, drop ", data.size(), " bytes");
        return false;
    }
    auto dc = m_dc_reliable;
    if (!dc) return false;

    try {
        dc->send(reinterpret_cast<const std::byte*>(data.data()), data.size());
        return true;
    }
    catch (const std::exception& e) {
        LOG(LOG_SCRIPTING, "[TanLobbyGameCtx] dc->send threw: ", e.what());
        return false;
    }
}

// unified exit: trigger Python event + optional process exit
void TanLobbyGameCtx::doExit(const char* event_name, int exit_code) {
    LOG(LOG_ERROR, "[TanLobbyGameCtx] doExit: event=", event_name, " code=", exit_code);
    PythonEventEngine e;
    e.trigger(event_name, exit_code);
    if (m_disout) {
        LOG(LOG_ERROR, "[TanLobbyGameCtx] disout enabled, exiting with code ", exit_code);
        exit(exit_code);
    }
}

void TanLobbyGameCtx::shutdown() {
    LOG(LOG_SCRIPTING, "[TanLobbyGameCtx] shutdown() called");
    m_closing.store(true, std::memory_order_release);
    m_running = false;
    stopKeepalive();
    m_dc_open = false;

    {
        std::lock_guard<std::mutex> lk(m_signaling_mutex);

        if (m_dc_reliable) {
            try { m_dc_reliable->close(); }
            catch (...) {}
            m_dc_reliable.reset();
        }
        if (m_dc_unreliable) {
            try { m_dc_unreliable->close(); }
            catch (...) {}
            m_dc_unreliable.reset();
        }
        if (m_pc) {
            try { m_pc->close(); }
            catch (...) {}
            m_pc.reset();
        }
    }

    if (m_ws_client) {
        m_ws_client.reset();
    }
}
void TanLobbyGameCtx::sendControlMessage(int type) {
    if (!m_ws_client || !m_ws_client->isConnected()) return;
    Json::Value root;
    root["Type"] = type;
    Json::StreamWriterBuilder b;
    b["indentation"] = "";
    m_ws_client->sendData(Json::writeString(b, root));
}

void TanLobbyGameCtx::sendSignalingMessage(const std::string& message_payload) {
    if (!m_ws_client || !m_ws_client->isConnected()) return;
    Json::Value root;
    root["Type"] = 1;

    uint64_t to_num = std::stoull(m_nethernet_id_to);
    root["To"] = static_cast<Json::UInt64>(to_num);

    root["From"] = m_nethernet_id_from;
    root["Message"] = message_payload;

    Json::StreamWriterBuilder b;
    b["indentation"] = "";
    m_ws_client->sendData(Json::writeString(b, root));
}

void TanLobbyGameCtx::startKeepalive() {
    if (m_keepalive_running.exchange(true)) return;
    std::weak_ptr<TanLobbyGameCtx> weak_self = shared_from_this();
    m_keepalive_thread = std::thread([weak_self]() {
        using namespace std::chrono;
        int64_t next_ping_at_sec = 0;
        LOG(LOG_SCRIPTING, "[TanLobbyGameCtx] keepalive thread started");
        while (true) {
            auto self = weak_self.lock();
            if (!self || !self->m_keepalive_running.load(std::memory_order_acquire)) break;
            if (!self->m_ws_client || !self->m_ws_client->isConnected()) {
                LOG(LOG_SCRIPTING, "[TanLobbyGameCtx] keepalive: ws closed, exit");
                break;
            }
            int64_t now_sec = duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
            if (now_sec >= next_ping_at_sec) {
                self->sendControlMessage(0);
                next_ping_at_sec = now_sec + 10;
            }
            if (!self->m_received_any.load(std::memory_order_acquire)) {
                LOG(LOG_SCRIPTING, "[TanLobbyGameCtx] keepalive: no data yet, sending Type=2");
                self->sendControlMessage(2);
            }
            self.reset();
            std::this_thread::sleep_for(seconds(2));
        }
        LOG(LOG_SCRIPTING, "[TanLobbyGameCtx] keepalive thread exited");

        auto self = weak_self.lock();
        if (self) {
            self->doExit("on_nethernet_invalid_token", EXIT_CODE_WS_Invalid_token);
        }
        });
}

void TanLobbyGameCtx::stopKeepalive() {
    m_keepalive_running.store(false, std::memory_order_release);
    if (m_keepalive_thread.joinable()) m_keepalive_thread.join();
}

void TanLobbyGameCtx::onConnection(bool connected) {
    LOG(LOG_SCRIPTING, "[TanLobbyGameCtx] onConnection connected=", connected);
    if (connected) {
        m_ws_connected.store(true, std::memory_order_release);
        startKeepalive();
        startTurnConfigTimeout();
    }
    else {
        doExit("on_nethernet_ws_closed", EXIT_CODE_WS_CONNECT);
    }
}

void TanLobbyGameCtx::onDataReceived(const std::string& content, size_t size) {
    m_received_any.store(true, std::memory_order_release);
    LOG(LOG_SCRIPTING, "[TanLobbyGameCtx] onDataReceived size=", size, " content=", content);
    handleIncomingMessage(content);
}

void TanLobbyGameCtx::handleIncomingMessage(const std::string& json_str) {
    Json::Value root;
    if (!parse_json(json_str, root)) {
        LOG(LOG_SCRIPTING, "[TanLobbyGameCtx] failed to parse incoming JSON");
        return;
    }
    int type = root.get("Type", -1).asInt();
    std::string from = root.get("From", "").asString();
    std::string message = root.get("Message", "").asString();
    switch (type) {
    case 2: handleType2_TurnConfig(message); break;
    case 1: handleType1_Signaling(from, message); break;
    default:
        LOG(LOG_SCRIPTING, "[TanLobbyGameCtx] unknown Type=", type);
        break;
    }
}

void TanLobbyGameCtx::handleType2_TurnConfig(const std::string& turn_config_json) {
    if (m_turn_received.exchange(true)) {
        LOG(LOG_SCRIPTING, "[TanLobbyGameCtx] Type=2 received again, ignored");
        return;
    }
    LOG(LOG_SCRIPTING, "[TanLobbyGameCtx] Type=2 TURN config received, setting up PeerConnection");
    setupPeerConnection(turn_config_json);
}

void TanLobbyGameCtx::setupPeerConnection(const std::string& turn_config_json) {
    Json::Value turn;
    if (!parse_json(turn_config_json, turn)) {
        LOG(LOG_SCRIPTING, "[TanLobbyGameCtx] failed to parse TURN config");
        return;
    }

    rtc::Configuration config;
    if (turn.isMember("TurnAuthServers") && turn["TurnAuthServers"].isArray()) {
        for (const auto& srv : turn["TurnAuthServers"]) {
            std::string user = srv.get("Username", "").asString();
            std::string pass = srv.get("Password", "").asString();
            std::string user_enc = url_encode(user);
            std::string pass_enc = url_encode(pass);
            if (!srv.isMember("Urls") || !srv["Urls"].isArray()) continue;
            for (const auto& u : srv["Urls"]) {
                std::string url = u.asString();
                if (url.rfind("turn:", 0) == 0) {
                    std::string host_port = url.substr(5);
                    std::string full = "turn:" + user_enc + ":" + pass_enc + "@" + host_port;
                    LOG(LOG_SCRIPTING, "[TanLobbyGameCtx] add ICE server: turn:***@", host_port);
                    config.iceServers.emplace_back(full);
                }
                else {
                    LOG(LOG_SCRIPTING, "[TanLobbyGameCtx] add ICE server: ", url);
                    config.iceServers.emplace_back(url);
                }
            }
        }
    }

    m_connection_id = std::to_string(GetRandomData());
    LOG(LOG_SCRIPTING, "[TanLobbyGameCtx] connection_id=", m_connection_id);

    std::lock_guard<std::mutex> lk(m_signaling_mutex);
    m_pc = std::make_shared<rtc::PeerConnection>(config);

    std::weak_ptr<TanLobbyGameCtx> weak_self = shared_from_this();

    m_pc->onLocalDescription([weak_self](rtc::Description desc) {
        auto self = weak_self.lock();
        if (!self) return;

        std::string sdp = static_cast<std::string>(desc);

        static const std::regex o_line_re(R"(\no=(\S+) (\d+) \d+ )");
        sdp = std::regex_replace(sdp, o_line_re, "\no=$1 $2 2 ");

        std::string type = desc.typeString();

        std::string payload = "CONNECTREQUEST " + self->m_connection_id + " " + sdp;
        self->sendSignalingMessage(payload);

        self->startConnectResponseTimeout();
        });

    m_pc->onLocalCandidate([weak_self](rtc::Candidate cand) {
        auto self = weak_self.lock();
        if (!self) return;
        std::string cand_str = static_cast<std::string>(cand);
        LOG(LOG_SCRIPTING, "[TanLobbyGameCtx] onLocalCandidate: ", cand_str);
        std::string payload = "CANDIDATEADD " + self->m_connection_id + " " + cand_str;
        self->sendSignalingMessage(payload);
        });

    m_pc->onStateChange([weak_self](rtc::PeerConnection::State state) {
        auto self = weak_self.lock();
        if (!self) return;
        LOG(LOG_SCRIPTING, "[TanLobbyGameCtx] PC state=", pc_state_name(state));

        if (state == rtc::PeerConnection::State::Connected) {
            self->m_pc_connected.store(true, std::memory_order_release);
        }
        else if (state == rtc::PeerConnection::State::Failed ||
            state == rtc::PeerConnection::State::Disconnected ||
            state == rtc::PeerConnection::State::Closed) {
            self->handleConnectionLost();
        }
        });

    m_pc->onGatheringStateChange([weak_self](rtc::PeerConnection::GatheringState s) {
        auto self = weak_self.lock();
        if (!self) return;
        LOG(LOG_SCRIPTING, "[TanLobbyGameCtx] PC gathering state=", static_cast<int>(s));
        });
    rtc::DataChannelInit unrel_init;
    unrel_init.reliability.unordered = true;
    unrel_init.reliability.maxRetransmits = 0;
    m_dc_unreliable = m_pc->createDataChannel("UnreliableDataChannel", unrel_init);

    m_dc_unreliable->onOpen([]() {
        LOG(LOG_SCRIPTING, "[TanLobbyGameCtx] UnreliableDataChannel opened (no business data)");
        });

    m_dc_unreliable->onMessage([](rtc::message_variant msg) {
        LOG(LOG_WARN, "[TanLobbyGameCtx] Unexpected message on UnreliableDataChannel");
        });

    rtc::DataChannelInit rel_init;
    m_dc_reliable = m_pc->createDataChannel("ReliableDataChannel", rel_init);

    m_dc_reliable->onOpen([weak_self]() {
        auto self = weak_self.lock();
        if (!self) return;
        LOG(LOG_SCRIPTING, "[TanLobbyGameCtx] DataChannel MC opened");

        self->m_dc_open = true;
        OnDataChannelOpen cb;
        {
            std::lock_guard<std::mutex> lk(self->m_callback_mutex);
            cb = self->m_on_dc_open;
        }
        if (cb) {
            try { cb(); }
            catch (const std::exception& e) {
                LOG(LOG_SCRIPTING, "[TanLobbyGameCtx] onOpen user callback threw: ", e.what());
            }
        }
        });

    m_dc_reliable->onClosed([weak_self]() {
        auto self = weak_self.lock();
        if (!self) return;
        LOG(LOG_SCRIPTING, "[TanLobbyGameCtx] DataChannel MC closed");

        self->m_dc_open = false;

        OnDataChannelClose cb;
        {
            std::lock_guard<std::mutex> lk(self->m_callback_mutex);
            cb = self->m_on_dc_close;
        }
        if (cb) {
            try { cb(); }
            catch (const std::exception& e) {
                LOG(LOG_SCRIPTING, "[TanLobbyGameCtx] onClose user callback threw: ", e.what());
            }
        }

        self->handleConnectionLost();
        });

    m_dc_reliable->onError([weak_self](std::string err) {
        auto self = weak_self.lock();
        if (!self) return;
        LOG(LOG_SCRIPTING, "[TanLobbyGameCtx] DataChannel error: ", err);
        });

    m_dc_reliable->onMessage([weak_self](rtc::message_variant msg) {
        auto self = weak_self.lock();
        if (!self) return;

        std::vector<uint8_t> data;
        if (auto* bin = std::get_if<rtc::binary>(&msg)) {
            data.reserve(bin->size());
            for (auto b : *bin) data.push_back(static_cast<uint8_t>(b));
        }
        else if (auto* str = std::get_if<std::string>(&msg)) {
            data.assign(str->begin(), str->end());
        }

        OnDataChannelMessage cb;
        {
            std::lock_guard<std::mutex> lk(self->m_callback_mutex);
            cb = self->m_on_dc_message;
        }
        if (cb && !data.empty()) {
            try { cb(data); }
            catch (const std::exception& e) {
                LOG(LOG_SCRIPTING, "[TanLobbyGameCtx] onMessage user callback threw: ", e.what());
            }
        }
        });
    startWebRtcTimeout();
}

void TanLobbyGameCtx::handleType1_Signaling(const std::string& from, const std::string& message) {
    if (from != m_nethernet_id_to) {
        LOG(LOG_SCRIPTING, "[TanLobbyGameCtx] Type=1 from unexpected sender: ", from);
        return;
    }

    auto firstSpace = message.find(' ');
    if (firstSpace == std::string::npos) return;
    auto secondSpace = message.find(' ', firstSpace + 1);
    if (secondSpace == std::string::npos) return;

    std::string cmd = message.substr(0, firstSpace);
    std::string connId = message.substr(firstSpace + 1, secondSpace - firstSpace - 1);
    std::string content = message.substr(secondSpace + 1);

    std::lock_guard<std::mutex> lk(m_signaling_mutex);
    if (!m_pc) {
        LOG(LOG_SCRIPTING, "[TanLobbyGameCtx] received ", cmd, " but PC not ready, drop");
        return;
    }

    try {
        if (cmd == "CONNECTRESPONSE") {
            LOG(LOG_SCRIPTING, "[TanLobbyGameCtx] handle CONNECTRESPONSE (answer)");
            m_connect_response_received.store(true, std::memory_order_release);
            m_pc->setRemoteDescription(rtc::Description(content, "answer"));
        }
        else if (cmd == "CANDIDATEADD") {
            LOG(LOG_SCRIPTING, "[TanLobbyGameCtx] handle CANDIDATEADD: ", content);
            m_pc->addRemoteCandidate(rtc::Candidate(content, "0"));
        }
        else {
            LOG(LOG_SCRIPTING, "[TanLobbyGameCtx] unknown signaling cmd: ", cmd);
        }
    }
    catch (const std::exception& e) {
        LOG(LOG_SCRIPTING, "[TanLobbyGameCtx] signaling exception: ", e.what());
    }
}

// Stage 1: WebSocket connect timeout
void TanLobbyGameCtx::startWsConnectTimeout() {
    std::weak_ptr<TanLobbyGameCtx> weak_self = shared_from_this();
    m_ws_timeout_thread = std::thread([weak_self]() {
        using namespace std::chrono;
        auto deadline = steady_clock::now() + milliseconds(WS_CONNECT_TIMEOUT_MS);

        while (steady_clock::now() < deadline) {
            auto self = weak_self.lock();
            if (!self) return;
            if (self->m_ws_connected.load(std::memory_order_acquire)) return;
            if (self->m_closing.load(std::memory_order_acquire)) return;
            if (!self->m_running) return;
            self.reset();
            std::this_thread::sleep_for(milliseconds(200));
        }

        auto self = weak_self.lock();
        if (!self) return;
        if (self->m_ws_connected.load(std::memory_order_acquire)) return;

        LOG(LOG_ERROR, "[TanLobbyGameCtx] WebSocket connect timeout after ", WS_CONNECT_TIMEOUT_MS, "ms");
        self->doExit("on_nethernet_ws_timeout", EXIT_CODE_WS_CONNECT);
        });
    m_ws_timeout_thread.detach();
}

// Stage 2: TURN config timeout
void TanLobbyGameCtx::startTurnConfigTimeout() {
    std::weak_ptr<TanLobbyGameCtx> weak_self = shared_from_this();
    m_turn_timeout_thread = std::thread([weak_self]() {
        using namespace std::chrono;
        auto deadline = steady_clock::now() + milliseconds(TURN_CONFIG_TIMEOUT_MS);

        while (steady_clock::now() < deadline) {
            auto self = weak_self.lock();
            if (!self) return;
            if (self->m_turn_received.load(std::memory_order_acquire)) return;
            if (self->m_closing.load(std::memory_order_acquire)) return;
            if (!self->m_running) return;
            self.reset();
            std::this_thread::sleep_for(milliseconds(200));
        }

        auto self = weak_self.lock();
        if (!self) return;
        if (self->m_turn_received.load(std::memory_order_acquire)) return;

        LOG(LOG_ERROR, "[TanLobbyGameCtx] TURN config timeout after ", TURN_CONFIG_TIMEOUT_MS, "ms");
        self->doExit("on_nethernet_turn_timeout", EXIT_CODE_TURN_TIMEOUT);
        });
    m_turn_timeout_thread.detach();
}

// Stage 3: CONNECTRESPONSE timeout
void TanLobbyGameCtx::startConnectResponseTimeout() {
    m_connect_response_received.store(false, std::memory_order_release);

    std::weak_ptr<TanLobbyGameCtx> weak_self = shared_from_this();
    m_connect_timeout_thread = std::thread([weak_self]() {
        using namespace std::chrono;
        auto deadline = steady_clock::now() + milliseconds(CONNECT_RESPONSE_TIMEOUT_MS);

        while (steady_clock::now() < deadline) {
            auto self = weak_self.lock();
            if (!self) return;
            if (self->m_connect_response_received.load(std::memory_order_acquire)) {
                LOG(LOG_SCRIPTING, "[TanLobbyGameCtx] CONNECTRESPONSE received, timeout watcher exit");
                return;
            }
            if (!self->m_running) return;
            self.reset();
            std::this_thread::sleep_for(milliseconds(100));
        }

        auto self = weak_self.lock();
        if (!self) return;
        if (self->m_connect_response_received.load(std::memory_order_acquire)) return;

        LOG(LOG_ERROR, "[TanLobbyGameCtx] CONNECTRESPONSE timeout after ", CONNECT_RESPONSE_TIMEOUT_MS, "ms");
        self->doExit("on_nethernet_connect_timeout", EXIT_CODE_WS_TIMEOUT);
        });
    m_connect_timeout_thread.detach();
}

// Stage 4: WebRTC connect timeout (ICE/DTLS/SCTP)
void TanLobbyGameCtx::startWebRtcTimeout() {
    std::weak_ptr<TanLobbyGameCtx> weak_self = shared_from_this();
    m_webrtc_timeout_thread = std::thread([weak_self]() {
        using namespace std::chrono;
        auto deadline = steady_clock::now() + milliseconds(WEBRTC_CONNECT_TIMEOUT_MS);

        while (steady_clock::now() < deadline) {
            auto self = weak_self.lock();
            if (!self) return;
            if (self->m_pc_connected.load(std::memory_order_acquire)) {
                LOG(LOG_SCRIPTING, "[TanLobbyGameCtx] PC connected, webrtc timeout watcher exit");
                return;
            }
            if (self->m_closing.load(std::memory_order_acquire)) return;
            if (!self->m_running) return;
            self.reset();
            std::this_thread::sleep_for(milliseconds(200));
        }

        auto self = weak_self.lock();
        if (!self) return;
        if (self->m_pc_connected.load(std::memory_order_acquire)) return;

        LOG(LOG_ERROR, "[TanLobbyGameCtx] WebRTC connect timeout after ",
            WEBRTC_CONNECT_TIMEOUT_MS, "ms (ICE/DTLS never connected)");
        self->doExit("on_nethernet_webrtc_timeout", EXIT_CODE_NetherNet_TIMEOUT);
        });
    m_webrtc_timeout_thread.detach();
}

// Stage 5: connection lost after established
void TanLobbyGameCtx::handleConnectionLost() {
    if (m_closing.load(std::memory_order_acquire)) return;
    if (m_connection_lost_reported.exchange(true)) return;

    LOG(LOG_ERROR, "[TanLobbyGameCtx] connection lost / closed unexpectedly");
    doExit("on_nethernet_disconnected", EXIT_CODE_NetherNet_TIMEOUT);
}

void TanLobbyGameCtx::startUp(const std::string& nethernet_id_to,
    const std::string& nethernet_id_from,
    const std::string& token,
    const std::string& server_ip,
    int server_port,
    uint32_t user_id) {
    LOG(LOG_SCRIPTING,
        "[TanLobbyGameCtx] startUp - nethernet_id_to=", nethernet_id_to,
        ", nethernet_id_from=", nethernet_id_from,
        ", token=", token,
        ", server_ip=", server_ip,
        ", server_port=", server_port,
        ", user_id=", user_id);

    m_nethernet_id_to = nethernet_id_to;
    m_nethernet_id_from = nethernet_id_from;
    m_token = token;
    m_server_ip = server_ip;
    m_server_port = server_port;
    m_user_id = user_id;

    std::string key_raw = Base64Cpp::base64_decode(token);
    if (key_raw.size() != 16) {
        LOG(LOG_SCRIPTING, "[TanLobbyGameCtx] ERROR: decoded token length=", key_raw.size(), ", expected 16");
        return;
    }
    uint8_t key[16];
    std::memcpy(key, key_raw.data(), 16);

    uint8_t plain[16];
    random_bytes(plain, 16);
    uint8_t cipher[16];
    if (!aes_ecb_128_encrypt_block(key, plain, cipher)) {
        LOG(LOG_SCRIPTING, "[TanLobbyGameCtx] ERROR: AES-128-ECB encryption failed");
        return;
    }

    std::string b64_plain = to_url_safe(Base64Cpp::base64_encode(plain, 16));
    std::string b64_cipher = to_url_safe(Base64Cpp::base64_encode(cipher, 16));

    std::string path = "/" + m_nethernet_id_from
        + "/" + std::to_string(m_user_id)
        + "/" + b64_plain
        + "/" + b64_cipher;
    LOG(LOG_SCRIPTING, "[TanLobbyGameCtx] full url=ws://", server_ip, ":", server_port, path);

    m_ws_client = std::make_unique<WebSocketClient>(server_ip, server_port, "Minecraft-Bedrock", path);

    auto self = shared_from_this();
    bool ok = m_ws_client->connect(
        [self](const std::string& content, size_t size) { self->onDataReceived(content, size); },
        [self](bool connected) { self->onConnection(connected); }
    );
    LOG(LOG_SCRIPTING, "[TanLobbyGameCtx] connect() returned ", ok);

    m_initialized = true;
    m_running = ok;

    if (ok) {
        startWsConnectTimeout();
    }
}

std::shared_ptr<TanLobbyGameCtx> TanLobbyGameCtx::create(bool disout) {
    LOG(LOG_SCRIPTING, "[TanLobbyGameCtx] create() factory called");
    return std::make_shared<TanLobbyGameCtx>(disout);
}

void register_tan_lobby_game_module() {
    static bool registered = false;
    if (registered) return;
    registered = true;
    if (Params::logger) {
        rtc::InitLogger(rtc::LogLevel::Debug);
    }
    LOG(LOG_SCRIPTING, "[PythonRuntime] Registering tan_lobby_game_clicpp_wrapper module...");
    // pybind11 2.13 placement-news the PyModuleDef onto the passed pointer, so a
    // nullptr here would write to address 0 and crash. Pass a real allocation.
    py::module m = py::module::create_extension_module(
        "tan_lobby_game_clicpp_wrapper", "Tan Lobby Game C++ Wrapper", new PyModuleDef());

    py::class_<TanLobbyGameCtx, std::shared_ptr<TanLobbyGameCtx>>(m, "TanLobbyGameCtx")
        .def("startUp", &TanLobbyGameCtx::startUp,
            py::arg("nethernet_id_to"),
            py::arg("nethernet_id_from"),
            py::arg("token"),
            py::arg("server_ip"),
            py::arg("server_port"),
            py::arg("user_id"))
        .def("__repr__", [](const TanLobbyGameCtx&) { return std::string("<TanLobbyGameCtx>"); });

    m.def("create", &TanLobbyGameCtx::create, py::arg("disout") = false);
    py::module::import("sys").attr("modules")["tan_lobby_game_clicpp_wrapper"] = m;
    LOG(LOG_SCRIPTING, "[PythonRuntime] tan_lobby_game_clicpp_wrapper registered!");
}
