#pragma once
#include <functional>
#include <vector>
#include <cstdint>
#include <unordered_map>
#include "PyEventHandler.h"

extern class ConnectInstance;
class PythonWrapperCallback
{
public:
    PythonWrapperCallback() {
        eventHandlers = std::unordered_map<uint32_t, std::vector<PyEventHandler>>();
    }
    ~PythonWrapperCallback() = default;

    void registerEventHandler(uint32_t sid, PyEventHandler handler) {
        eventHandlers[sid].push_back(handler);
    }

    void invokeEventHandlers(uint32_t sid, const std::string& data) {
        auto it = eventHandlers.find(sid);
        if (it != eventHandlers.end()) {
            for (auto& handler : it->second) {
                handler.invokeCallback(data);
            }
        }
    }
    void clearUserEvent() {
        Logger::getInstance().logv(LOG_INFO, "PythonWrapperCallback: clear all user events.");
        for (auto it = eventHandlers.begin(); it != eventHandlers.end(); ) {
            auto& handlers = it->second;

            // 清理内部 vector
            for (size_t j = 0; j < handlers.size(); ) {
                if (!handlers[j].isKernel()) {
                    handlers.erase(handlers.begin() + j);
                }
                else {
                    ++j;
                }
            }

            // 空了就删掉这一项，erase 会返回新迭代器
            if (handlers.empty()) {
                it = eventHandlers.erase(it);
            }
            else {
                ++it;
            }
        }
    }
    void clearKernelEvent() {
        Logger::getInstance().logv(LOG_INFO, "PythonWrapperCallback: clear all kernel events.");
        for (auto it = eventHandlers.begin(); it != eventHandlers.end(); ) {
            auto& handlers = it->second;

            // 清理内部 vector
            for (size_t j = 0; j < handlers.size(); ) {
                if (handlers[j].isKernel()) {
                    handlers.erase(handlers.begin() + j);
                }
                else {
                    ++j;
                }
            }

            // 空了就删掉这一项，erase 会返回新迭代器
            if (handlers.empty()) {
                it = eventHandlers.erase(it);
            }
            else {
                ++it;
            }
        }
    }

    void clearAllEvent() {
        Logger::getInstance().logv(LOG_INFO, "PythonWrapperCallback: clear all events.");
        eventHandlers.clear();
    }

private:
    std::unordered_map<uint32_t, std::vector<PyEventHandler>> eventHandlers;
};

