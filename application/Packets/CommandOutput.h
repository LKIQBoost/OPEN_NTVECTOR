#pragma once
#include "PacketBase.h"
#include <array>

// OutputType 常量定义
constexpr uint8_t CommandOutputTypeNone = 0;
constexpr uint8_t CommandOutputTypeLastOutput = 1;
constexpr uint8_t CommandOutputTypeSilent = 2;
constexpr uint8_t CommandOutputTypeAllOutput = 3;
constexpr uint8_t CommandOutputTypeDataSet = 4;

// CommandOrigin 类型常量
constexpr uint32_t CommandOriginPlayer = 0;
constexpr uint32_t CommandOriginBlock = 1;
constexpr uint32_t CommandOriginMinecartBlock = 2;
constexpr uint32_t CommandOriginDevConsole = 3;
constexpr uint32_t CommandOriginTest = 4;
constexpr uint32_t CommandOriginAutomationPlayer = 5;
constexpr uint32_t CommandOriginClientAutomation = 6;
constexpr uint32_t CommandOriginDedicatedServer = 7;
constexpr uint32_t CommandOriginEntity = 8;
constexpr uint32_t CommandOriginVirtual = 9;
constexpr uint32_t CommandOriginGameArgument = 10;
constexpr uint32_t CommandOriginEntityServer = 11;
constexpr uint32_t CommandOriginPrecompiled = 12;
constexpr uint32_t CommandOriginGameDirectorEntityServer = 13;
constexpr uint32_t CommandOriginScript = 14;
constexpr uint32_t CommandOriginExecutor = 15;

// 命令来源结构
struct CommandOrigin {
    uint32_t Origin = 0;              // 命令来源类型
    std::array<uint8_t, 16> UUID;   // UUID (16 bytes)
    std::string RequestID;            // 请求 ID
    int64_t PlayerUniqueID = 0;       // 仅当 Origin == DevConsole 或 Test 时存在
};

// 命令输出消息结构
struct CommandOutputMessage {
    bool Success = false;                  // 是否成功
    std::string Message;                   // 消息内容
    std::vector<std::string> Parameters;   // 参数列表
};

class CommandOutput : public PacketBase
{
public:
    unsigned char ID() override;
    void Deserializ(std::vector<unsigned char> pack) override;
    std::vector<unsigned char> Serializ() override;

    // 数据包字段
    CommandOrigin Origin;                          // 命令来源
    uint8_t OutputType = 0;                        // 输出类型
    uint32_t SuccessCount = 0;                     // 成功次数
    std::vector<CommandOutputMessage> OutputMessages;  // 输出消息列表
    std::string DataSet;                           // 仅当 OutputType == 4 时存在

    // 便捷方法：获取所有消息字符串
    std::vector<std::string> GetAllMessages() const;
};

