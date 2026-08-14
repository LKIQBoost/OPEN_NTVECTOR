// websocket_client_interface.h
#pragma once
#include <string>
#include <functional>
#include <memory>
/*
这是一个空壳基类知道吧
因为傻逼python2.7太傻逼了
所以用了这样一个刁钻的方法去调用websocket
我是天才！
*/

// 空壳基类 - 完全不包含 libwebsockets 的任何痕迹
class IWebSocketClient {
public:
    // 回调类型定义
    using OnDataReceived = std::function<void(const std::string&, size_t)>;
    using OnConnectionState = std::function<void(bool)>;

    // 工厂函数 - 唯一创建实例的方式
    static std::unique_ptr<IWebSocketClient> Create(
        const std::string& serverIp,
        int serverPort,
        const std::string& protocol = "ws-protocol",
        const std::string& path = "/");

    virtual ~IWebSocketClient() = default;

    // 纯虚接口
    virtual bool connect(OnDataReceived onDataCb = nullptr,
        OnConnectionState onConnCb = nullptr) = 0;
    virtual void disconnect() = 0;
    virtual bool sendData(const std::string& data) = 0;
    virtual bool isConnected() const = 0;

protected:
    // 禁止外部直接构造/拷贝
    IWebSocketClient() = default;
    IWebSocketClient(const IWebSocketClient&) = delete;
    IWebSocketClient& operator=(const IWebSocketClient&) = delete;
};