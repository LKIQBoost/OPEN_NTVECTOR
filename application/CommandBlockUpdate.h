#pragma once
#include "PacketBase.h"

// 命令方块模式常量
constexpr uint32_t CommandBlockModeImpulse = 0;    // 脉冲模式
constexpr uint32_t CommandBlockModeRepeating = 1;  // 循环模式
constexpr uint32_t CommandBlockModeChain = 2;      // 链模式

// 方块位置结构 (整数坐标)
struct BlockPos {
    int32_t x = 0;
    int32_t y = 0;
    int32_t z = 0;
};

// CommandBlockUpdate 数据包
// 客户端发送此包来更新指定位置的命令方块
// 命令方块可以是物理方块或矿车中的命令方块
class CommandBlockUpdate : public PacketBase
{
public:
    unsigned char ID() override;
    void Deserializ(std::vector<unsigned char> pack) override;
    std::vector<unsigned char> Serializ() override;

    // 数据包字段
    bool Block = true;                    // 是否为物理方块 (false 表示矿车中的命令方块)

    // 以下字段仅当 Block == true 时有效
    BlockPos Position;                    // 命令方块位置
    uint32_t Mode = 0;                    // 命令方块模式 (Impulse/Repeating/Chain)
    bool NeedsRedstone = false;           // 是否需要红石激活
    bool Conditional = false;             // 是否为条件模式

    // 以下字段仅当 Block == false 时有效
    uint64_t MinecartEntityRuntimeID = 0; // 矿车实体运行时 ID

    // 通用字段
    std::string Command;                  // 命令内容
    std::string LastOutput;               // 上次输出
    std::string Name;                     // 命令方块名称 (悬停显示)
    std::string FilteredName;             // 过滤后的名称（脏话过滤版，可为空）
    bool ShouldTrackOutput = false;       // 是否跟踪输出
    uint32_t TickDelay = 0;               // 执行延迟 (tick, 网易版为 uint32)
    bool ExecuteOnFirstTick = false;      // 是否在第一个 tick 执行

    // 便捷方法
    bool IsImpulseMode() const { return Mode == CommandBlockModeImpulse; }
    bool IsRepeatingMode() const { return Mode == CommandBlockModeRepeating; }
    bool IsChainMode() const { return Mode == CommandBlockModeChain; }
    const char* GetModeString() const;
};
