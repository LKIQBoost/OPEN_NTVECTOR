#include "WebSockeVirtualWrapper.h"
// WebSocketClient.cpp

// 构造函数 - 直接创建底层实现
WebSocketClient::WebSocketClient(const std::string& serverIp,
    int serverPort,
    const std::string& protocol,
    const std::string& path)
    : m_impl(IWebSocketClient::Create(serverIp, serverPort, protocol, path)) {
    // 啥也不用干，全部转发给m_impl
}

// 析构函数
WebSocketClient::~WebSocketClient() {
    // m_impl 自动销毁
}

// connect - 完美转发
bool WebSocketClient::connect(OnDataReceived onDataCb, OnConnectionState onConnCb) {
    if (!m_impl) return false;
    return m_impl->connect(std::move(onDataCb), std::move(onConnCb));
}

// disconnect - 完美转发
void WebSocketClient::disconnect() {
    if (m_impl) {
        m_impl->disconnect();
    }
}

// sendData - 完美转发
bool WebSocketClient::sendData(const std::string& data) {
    if (!m_impl) return false;
    return m_impl->sendData(data);
}

// isConnected - 完美转发
bool WebSocketClient::isConnected() const {
    if (!m_impl) return false;
    return m_impl->isConnected();
}