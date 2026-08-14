#pragma once
#include "PacketBase.h"

// Respawn 状态常量
constexpr uint8_t RespawnStateSearchingForSpawn = 0;   // 正在搜索重生点
constexpr uint8_t RespawnStateReadyToSpawn = 1;        // 服务端准备好重生
constexpr uint8_t RespawnStateClientReadyToSpawn = 2;  // 客户端准备好重生

// Respawn 数据包
// 服务端发送此包让玩家在客户端重生
// 客户端也会发送此包作为响应完成重生流程
class Respawn : public PacketBase
{
public:
    unsigned char ID() override;
    void Deserializ(std::vector<unsigned char> pack) override;
    std::vector<unsigned char> Serializ() override;

    // 数据包字段
    Vec3 Position;              // 重生位置 (可能在不同维度)
    uint8_t State = 0;          // 重生状态 (见上方常量)
    uint64_t EntityRuntimeID = 0;  // 实体运行时 ID

    // 便捷方法
    bool IsSearchingForSpawn() const { return State == RespawnStateSearchingForSpawn; }
    bool IsReadyToSpawn() const { return State == RespawnStateReadyToSpawn; }
    bool IsClientReadyToSpawn() const { return State == RespawnStateClientReadyToSpawn; }

    // 获取状态字符串
    const char* GetStateString() const;
};
