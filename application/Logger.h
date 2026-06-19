#pragma once
#include <string>
#include <iostream>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <map>
#include <ctime>
#include <cstdarg>
#include <thread>

#include "StartupParams.h"
#include "platform/PlatformCompat.h"
// ��־���궨��
#define LOG_INFO       (1 << 0)
#define LOG_WARN       (1 << 1)
#define LOG_ERROR      (1 << 2)
#define LOG_PLATFORM   (1 << 3)
#define LOG_ENTITY     (1 << 4)
#define LOG_DATABASE   (1 << 5)
#define LOG_GUI        (1 << 6)
#define LOG_SYSTEM     (1 << 7)
#define LOG_NETWORK    (1 << 8)
#define LOG_RENDER     (1 << 9)
#define LOG_MEMORY     (1 << 10)
#define LOG_ANIMATION  (1 << 11)
#define LOG_INPUT      (1 << 12)
#define LOG_LEVEL      (1 << 13)
#define LOG_SERVER     (1 << 14)
#define LOG_DLC        (1 << 15)
#define LOG_PHYSICS    (1 << 16)
#define LOG_FILE       (1 << 17)
#define LOG_STORAGE    (1 << 18)
#define LOG_REALMS     (1 << 19)
#define LOG_REALMSAPI  (1 << 20)
#define LOG_XBOXLIVE   (1 << 21)
#define LOG_USERMANAGER (1 << 22)
#define LOG_XSAPI      (1 << 23)
#define LOG_PERF       (1 << 24)
#define LOG_BLOCKS     (1 << 25)
#define LOG_TELEMETRY  (1 << 26)
#define LOG_RAKNET     (1 << 27)
#define LOG_SOUND      (1 << 28)
#define LOG_SCRIPTING  (1 << 29)
#define LOG_ITEMS      (1 << 30)
#define LOG_EDITOR     (1 << 31)

// ANSI ��ɫ����
/*
namespace ConsoleColor {
    const std::string RESET = "\033[0m";
    const std::string BLACK = "\033[0m";
    const std::string RED = "\033[0m";
    const std::string GREEN = "\033[0m";
    const std::string YELLOW = "\033[0m";
    const std::string BLUE = "\033[0m";
    const std::string MAGENTA = "\033[0m";
    const std::string CYAN = "\033[0m";
    const std::string WHITE = "\033[0m";
    const std::string GRAY = "\033[0m";
    const std::string BRIGHT_RED = "\033[0m";
    const std::string BRIGHT_GREEN = "\033[0m";
    const std::string BRIGHT_YELLOW = "\033[0m";
    const std::string BRIGHT_BLUE = "\033[0m";
    const std::string BRIGHT_MAGENTA = "\033[0m";
    const std::string BRIGHT_CYAN = "\033[0m";
    const std::string BRIGHT_WHITE = "\033[0m";
}*/
namespace ConsoleColor {
    // ������������
    const std::string RESET = "\033[0m";

    // ǰ��ɫ��������ɫ��
    const std::string BLACK = "\033[30m";
    const std::string RED = "\033[31m";
    const std::string GREEN = "\033[32m";
    const std::string YELLOW = "\033[33m";
    const std::string BLUE = "\033[34m";
    const std::string MAGENTA = "\033[35m";
    const std::string CYAN = "\033[36m";
    const std::string WHITE = "\033[37m";

    // ��ɫ
    const std::string BRIGHT_BLACK = "\033[90m";
    const std::string BRIGHT_RED = "\033[91m";
    const std::string BRIGHT_GREEN = "\033[92m";
    const std::string BRIGHT_YELLOW = "\033[93m";
    const std::string BRIGHT_BLUE = "\033[94m";
    const std::string BRIGHT_MAGENTA = "\033[95m";
    const std::string BRIGHT_CYAN = "\033[96m";
    const std::string BRIGHT_WHITE = "\033[97m";

    // ����ɫ
    const std::string BG_BLACK = "\033[40m";
    const std::string BG_RED = "\033[41m";
    const std::string BG_GREEN = "\033[42m";
    const std::string BG_YELLOW = "\033[43m";
    const std::string BG_BLUE = "\033[44m";
    const std::string BG_MAGENTA = "\033[45m";
    const std::string BG_CYAN = "\033[46m";
    const std::string BG_WHITE = "\033[47m";

    // ��ʽ
    const std::string BOLD = "\033[1m";
    const std::string DIM = "\033[2m";
    const std::string ITALIC = "\033[3m";
    const std::string UNDERLINE = "\033[4m";
    const std::string BLINK = "\033[5m";
    const std::string REVERSE = "\033[7m";
    const std::string HIDDEN = "\033[8m";
}

class Logger {
public:
    static Logger& getInstance();

    void initialize(uint32_t enabledCategories = 0xFFFFFFFF);
    void setCategoryColor(uint32_t category, const std::string& color);
    void enableCategory(uint32_t category);
    void disableCategory(uint32_t category);

    template<typename... Args>
    void log(uint32_t category, Args... args) {
        if (Params::logger) {
            if (!(category & enabledCategories_)) return;

            std::lock_guard<std::mutex> lock(mutex_);

            auto now = std::chrono::system_clock::now();
            auto now_time = std::chrono::system_clock::to_time_t(now);
            auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()) % 1000;

            std::ostringstream oss;

            // ��������ɫ
            auto it = categoryColors_.find(category);
            if (it != categoryColors_.end()) {
                oss << it->second;
            }

            // ʹ���̰߳�ȫ�� localtime_r
            std::tm time_info;
#ifdef _WIN32
            localtime_s(&time_info, &now_time);  // Windows
#else
            localtime_r(&now_time, &time_info);  // Linux/Unix
#endif
            /*
            // ��ȡ��ǰ����ID
            DWORD processId = GetCurrentProcessId();
            std::cout << "����ID (GetCurrentProcessId): " << processId << std::endl;

            // ��ȡ��ǰ�߳�ID
            DWORD threadId = GetCurrentThreadId();
            std::cout << "�߳�ID (GetCurrentThreadId): " << threadId << std::endl;
            */
            /*
            
    // ��ȡ����ID - ʹ��CRT��������ƽ̨������Windows��ʵ�ʵ���WinAPI��
    int pid = _getpid();  // ��Windows���ڲ�����GetCurrentProcessId()
    std::cout << "����ID (_getpid): " << pid << std::endl;
    
    // �߳�ID��CRT��û��ֱ�Ӷ�Ӧ����
    std::this_thread::get_id() ��C++11�ķ�ʽ
            */

            // Ȼ�������������ݣ�����ʱ��������
            oss << std::put_time(&time_info, "[%Y-%m-%d %H:%M:%S:")
                << std::setfill('0') << std::setw(3) << now_ms.count() << " "
                << getCategoryName(category) << " " << std::this_thread::get_id() << " " << PLATFORM_GETPID() << "]";

            // ������־����
            using expander = int[];
            (void)expander {
                0, (void(oss << args), 0)...
            };

            // ���������ɫ
            oss << ConsoleColor::RESET;

            std::cout << oss.str() << std::endl;
        }
    }
    void logv(uint32_t category, const char* format, ...);
    void log_(char* logger, uint32_t category = LOG_INFO) {

        // ��������ɫ
        auto it = categoryColors_.find(category);
        if (it != categoryColors_.end()) {
            std::cout << it->second;
        }
        if (Params::logger)
            std::cout << logger << ConsoleColor::RESET << '\n';
    }
    void log_(std::string logger, uint32_t category = LOG_INFO) {

        // ��������ɫ
        auto it = categoryColors_.find(category);
        if (it != categoryColors_.end()) {
            std::cout << it->second;
        }
        if (Params::logger)
            std::cout << "NO LOG FILE! - " << logger << ConsoleColor::RESET << '\n';
    }

private:
    Logger();
    ~Logger() = default;

    std::string getCategoryName(uint32_t category) const;

    uint32_t enabledCategories_;
    std::mutex mutex_;
    std::unordered_map<uint32_t, std::string> categoryColors_;
    std::unordered_map<uint32_t, std::string> categoryNames_;
};

// ��ݺ궨��
#define LOG(category, ...) Logger::getInstance().log(category, __VA_ARGS__)

