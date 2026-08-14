#include "SocketCallback.h"
#include <sstream>
#include <iomanip>

SocketCallback::SocketCallback(ConnectInstance* ctx)
    : lostConnectCallback_(nullptr) {
    m_ctx = ctx;
}

void SocketCallback::setLostConnectCallback(LostConnectCallback callback) {
    lostConnectCallback_ = callback;
}

void SocketCallback::registerReceiveCallback(uint32_t sid, ReceiveCallback callback) {
    receiveCallbacks_.emplace(sid, callback);
}

void SocketCallback::removeReceiveCallback(uint32_t sid) {
    receiveCallbacks_.erase(sid);
}

bool SocketCallback::invokeCallback(uint32_t sid, const std::vector<uint8_t>& paramlist) {
    auto it = receiveCallbacks_.find(sid);
    if (it == receiveCallbacks_.end()) {
        return false;
    }
    // 执行回调
    try {
        it->second(m_ctx, paramlist);
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "CallBackException(sid=" << (int)sid << "): " << e.what() << std::endl;
        return false;
    }
}

bool SocketCallback::invokeCallback(uint32_t sid, const uint8_t* data, size_t length) {
    std::vector<uint8_t> paramlist(data, data + length);
    return invokeCallback(sid, paramlist);
}

std::vector<uint32_t> SocketCallback::getRegisteredIds() const {
    std::vector<uint32_t> ids;
    for (const auto& pair : receiveCallbacks_) {
        ids.push_back(pair.first);
    }
    return ids;
}