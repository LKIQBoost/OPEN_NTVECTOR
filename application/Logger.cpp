#include "Logger.h"
#include <iostream>

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

Logger::Logger() {
    // 初始化类别名称映射
    categoryNames_ = {
        {LOG_INFO,      "INFO"},
        {LOG_WARN,      "WARN"},
        {LOG_ERROR,     "ERROR"},
        {LOG_PLATFORM,  "PLATFORM"},
        {LOG_ENTITY,    "ENTITY"},
        {LOG_DATABASE,  "DATABASE"},
        {LOG_GUI,       "GUI"},
        {LOG_SYSTEM,    "SYSTEM"},
        {LOG_NETWORK,   "NETWORK"},
        {LOG_RENDER,    "RENDER"},
        {LOG_MEMORY,    "MEMORY"},
        {LOG_ANIMATION, "ANIMATION"},
        {LOG_INPUT,     "INPUT"},
        {LOG_LEVEL,     "LEVEL"},
        {LOG_SERVER,    "SERVER"},
        {LOG_DLC,       "DLC"},
        {LOG_PHYSICS,   "PHYSICS"},
        {LOG_FILE,      "FILE"},
        {LOG_STORAGE,   "STORAGE"},
        {LOG_REALMS,    "REALMS"},
        {LOG_REALMSAPI, "REALMSAPI"},
        {LOG_XBOXLIVE,  "XBOXLIVE"},
        {LOG_USERMANAGER, "USERMANAGER"},
        {LOG_XSAPI,     "XSAPI"},
        {LOG_PERF,      "PERF"},
        {LOG_BLOCKS,    "BLOCKS"},
        {LOG_TELEMETRY, "TELEMETRY"},
        {LOG_RAKNET,    "RAKNET"},
        {LOG_SOUND,     "SOUND"},
        {LOG_SCRIPTING, "SCRIPTING"},
        {LOG_ITEMS,     "ITEMS"},
        {LOG_EDITOR,    "EDITOR"}
    };

    // 设置默认颜色
    categoryColors_ = {
        {LOG_INFO,      ConsoleColor::BRIGHT_WHITE},
        {LOG_WARN,      ConsoleColor::BRIGHT_YELLOW},
        {LOG_ERROR,     ConsoleColor::BRIGHT_RED},
        {LOG_PLATFORM,  ConsoleColor::RESET},
        {LOG_SYSTEM,    ConsoleColor::RESET},
        {LOG_NETWORK,   ConsoleColor::RESET}
        // 其他类别使用默认颜色
    };

    enabledCategories_ = 0xFFFFFFFF; // 默认启用所有类别
}

void Logger::initialize(uint32_t enabledCategories) {
    enabledCategories_ = enabledCategories;
}

void Logger::setCategoryColor(uint32_t category, const std::string& color) {
    std::lock_guard<std::mutex> lock(mutex_);
    categoryColors_[category] = color;
}

void Logger::enableCategory(uint32_t category) {
    std::lock_guard<std::mutex> lock(mutex_);
    enabledCategories_ |= category;
}

void Logger::disableCategory(uint32_t category) {
    std::lock_guard<std::mutex> lock(mutex_);
    enabledCategories_ &= ~category;
}
void Logger::logv(uint32_t category, const char* format, ...) {
    if (!Params::logger) return;

    va_list args;
    va_start(args, format);

    // 计算需要的缓冲区大小
    va_list args_copy;
    va_copy(args_copy, args);
    int size = vsnprintf(nullptr, 0, format, args_copy);
    va_end(args_copy);

    if (size > 0) {
        std::vector<char> buffer(size + 1);
        vsnprintf(buffer.data(), buffer.size(), format, args);

        // 调用模板 log 函数，把格式化后的字符串传进去
        log(category, std::string(buffer.data()));
    }

    va_end(args);
}

std::string Logger::getCategoryName(uint32_t category) const {
    auto it = categoryNames_.find(category);
    if (it != categoryNames_.end()) {
        return it->second;
    }
    return "UNKNOWN";
}
