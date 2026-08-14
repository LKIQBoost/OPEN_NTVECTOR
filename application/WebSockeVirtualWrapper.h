// WebSocketClient.h - 用户直接使用的类
#pragma once
#include <string>
#include <functional>
#include <memory>
#include "WebSocketClient.h"

// 完全一样的API风格，但底层调用IWebSocketClient
class WebSocketClient {
public:
    // 回调类型定义 - 和原来一模一样
    using OnDataReceived = std::function<void(const std::string&, size_t)>;
    using OnConnectionState = std::function<void(bool)>;

    // 构造函数 - 和原来一模一样！
    WebSocketClient(const std::string& serverIp,
        int serverPort,
        const std::string& protocol = "ws-protocol",
        const std::string& path = "/");

    ~WebSocketClient();

    // 禁止拷贝 - 和原来一模一样
    WebSocketClient(const WebSocketClient&) = delete;
    WebSocketClient& operator=(const WebSocketClient&) = delete;
    WebSocketClient(WebSocketClient&&) = delete;
    WebSocketClient& operator=(WebSocketClient&&) = delete;

    // 接口函数 - 和原来一模一样
    bool connect(OnDataReceived onDataCb = nullptr, OnConnectionState onConnCb = nullptr);
    void disconnect();
    bool sendData(const std::string& data);
    bool isConnected() const;

private:
    // 唯一的不同：底层用IWebSocketClient
    std::unique_ptr<IWebSocketClient> m_impl;
};