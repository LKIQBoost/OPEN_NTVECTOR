#pragma once
#include <functional>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <string>
#include <iostream>
#include <mutex>

extern class ConnectInstance;
class SocketCallback {
public:
    // 连接丢失回调函数类型
    using LostConnectCallback = std::function<void(const std::string&)>;

    // 数据接收回调函数类型
    using ReceiveCallback = std::function<void(ConnectInstance*, const std::vector<uint8_t>&)>;

    SocketCallback(ConnectInstance*);
    ~SocketCallback() = default;

    // 设置连接丢失回调
    void setLostConnectCallback(LostConnectCallback callback);

    // 注册接收回调（按消息ID）
    void registerReceiveCallback(uint32_t sid, ReceiveCallback callback);

    // 移除接收回调
    void removeReceiveCallback(uint32_t sid);

    // 执行回调（处理接收到的数据）
    bool invokeCallback(uint32_t sid, const std::vector<uint8_t>& paramlist);

    // 执行回调（字节数组版本）
    bool invokeCallback(uint32_t sid, const uint8_t* data, size_t length);

    // 获取已注册的消息ID列表
    std::vector<uint32_t> getRegisteredIds() const;

private:
    ConnectInstance* m_ctx;
    LostConnectCallback lostConnectCallback_;
    std::unordered_map<uint32_t, ReceiveCallback> receiveCallbacks_;
};