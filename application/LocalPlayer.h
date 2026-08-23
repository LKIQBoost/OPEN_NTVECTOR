#pragma once
#include "BinaryWriter.h"
#include "PlayerAuthInput.h"
class LocalPlayer
{
public:
    LocalPlayer() {
        memset(this, 0, sizeof(*this));
        inputData = std::vector<PlayerAuthInputData>();
    }
    __int64 EntityRuntimeID;
    Vec2 headRotation;//Pitch 和 Yaw 表示玩家报告的旋转角度
    Vec3 position;
    Vec2 MoveVector;//表示玩家报告的移动向量，通常是一个二维向量，表示玩家在水平面上的移动方向和速度（动量）
    float headYaw;//表示玩家报告的头部水平旋转角度
    std::vector<PlayerAuthInputData> inputData;//表示玩家的输入数据，通常是一个包含多个 PlayerAuthInputData 枚举值的列表，表示玩家当前的输入状态
    bool readyPosDeltaDirty;//是否衰落伤害，false是有伤害
};

