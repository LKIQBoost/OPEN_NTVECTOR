#pragma once
#include "PacketBase.h"
#include <iostream>
enum PlayerAuthInputData
{
    ASCEND,//上升
    DESCEND,//下降
    NORTH_JUMP,//向北跳跃
    JUMP_DOWN,//跳跃键按下
    SPRINT_DOWN,//疾跑键按下
    CHANGE_HEIGHT,//改变高度
    JUMPING,//正在跳跃
    AUTO_JUMPING_IN_WATER,//水中自动跳跃
    SNEAKING,//正在潜行
    SNEAK_DOWN,//潜行键按下
    UP,//向前移动
    DOWN,//向后移动
    LEFT,//向左移动
    RIGHT,//向右移动
    UP_LEFT,//左前移动
    UP_RIGHT,//右前移动
    WANT_UP,//希望上升
    WANT_DOWN,//希望下降
    WANT_DOWN_SLOW,//希望慢速下降
	WANT_UP_SLOW,//希望慢速上升
    SPRINTING,//正在疾跑
    ASCEND_SCAFFOLDING,//沿方块上升
    DESCEND_SCAFFOLDING,//沿方块下降
    SNEAK_TOGGLE_DOWN,//潜行切换
    PERSIST_SNEAK,//持续潜行
    START_SPRINTING,//开始疾跑
    STOP_SPRINTING,//停止疾跑
    START_SNEAKING,//开始潜行
    STOP_SNEAKING,//停止潜行
    START_SWIMMING,//开始游泳
    STOP_SWIMMING,//停止游泳
    START_JUMPING,//起跳瞬间
    START_GLIDING,//开始滑翔
    STOP_GLIDING,//停止滑翔
    PERFORM_ITEM_INTERACTION,//物品交互
    PERFORM_BLOCK_ACTIONS,//方块操作
    PERFORM_ITEM_STACK_REQUEST//物品堆请求
};
enum InputMode {
    UNDEFINED,//未知
    MOUSE,//鼠标
    TOUCH,//触摸
    GAMEPAD,//手柄
    MOTION_CONTROLLER//运动控制器
};

class PlayerAuthInput :
    public PacketBase
{
public:
    unsigned char ID() override {
        return IDPlayerAuthInput;
    }
    //void Deserializ(std::vector<unsigned char> pack) override;
    std::vector<unsigned char> Serializ() override {
		BinaryWriter bw(256);
        bw.WriteVarUInt32(ID());
		bw.WriteVec2(headRotation);
		bw.WriteVec3(position);
		bw.WriteVec2(MoveVector);
		bw.WriteFloat(headYaw);
        unsigned __int64 KeyMode = 0;
        // 第一步：只遍历，完成位运算（不修改容器）
        for (PlayerAuthInputData data : this->inputData) {
            KeyMode |= ((unsigned __int64)1 << (int)data);
        }
		bw.WriteVarUInt64(KeyMode);
		bw.WriteVarUInt(InputMode);
		bw.WriteVarUInt(playMode);
		bw.WriteVarUInt(interactionModel);
		bw.WriteFloat(interactPitch);
		bw.WriteFloat(interactYaw);
		bw.WriteVarInt64(Tick);
		bw.WriteVec3(delta);
        bw.WriteBool(bypassMoveVector);
		bw.WriteVec2(analogueMoveVector);
		bw.WriteVec3(cameraOrientation);
		bw.WriteVec2(rawMoveVector);
		bw.WriteBool(cameraDeparted);
		bw.WriteFloat(thirdPersonPerspective);
		bw.WriteFloat(playerRotationToCamera);
		bw.WriteBool(readyPosDeltaDirty);
		bw.WriteBool(isOnGround);
        bw.WriteBool(resetPosition);
        return bw.vect();
    }
    Vec2 headRotation;//Pitch 和 Yaw 表示玩家报告的旋转角度
    Vec3 position;//表示玩家报告的当前位置
	Vec2 MoveVector;//表示玩家报告的移动向量，通常是一个二维向量，表示玩家在水平面上的移动方向和速度（动量）
    float headYaw;//表示玩家报告的头部水平旋转角度
	std::vector<PlayerAuthInputData> inputData;//表示玩家的输入数据，通常是一个包含多个 PlayerAuthInputData 枚举值的列表，表示玩家当前的输入状态
	InputMode InputMode;//表示玩家使用的输入设备类型，通常是一个 InputMode 枚举值，表示玩家是使用鼠标、触摸屏、手柄还是其他输入设备
    int playMode;//指定玩家的游玩方式，它的值可能比较随机，可在上方找到
    int interactionModel;//表示玩家使用的交互模型，是上方常量之一
    float interactPitch;//用于物体交互的视角方向，例如 VR 或自定义摄像机 3.7新增
	float interactYaw;//用于物体交互的视角方向，例如 VR 或自定义摄像机 3.7新增
    __int64 Tick;//Tick表示发送该包时的服务器tick，用于CorrectPlayerMovePrediction
    Vec3 delta;//表示旧位置和新位置的差值，该字段实际无太大用途，服务器可以自行计算
	bool bypassMoveVector;//表示是否绕过服务器对 MoveVector 的预测和校正，直接使用客户端报告的 MoveVector 进行移动计算
    Vec2 analogueMoveVector;//AnalogueMoveVector 是一个 Vec2，指定玩家移动方向，由模拟输入生成的 X / Z 组合值。
	Vec3 cameraOrientation;//表示玩家报告的摄像机方向，通常是一个三维向量，表示玩家当前的摄像机朝向
    Vec2 rawMoveVector;//表示原始的输入移动值
    /*以下字段均为网易特有字段*/
    bool cameraDeparted;//是否开启分离相机
    float thirdPersonPerspective;//unknown
    float playerRotationToCamera;//unknown
    bool readyPosDeltaDirty;//是否衰落伤害，false是有伤害
    bool isOnGround;//玩家当前是否在地面
	bool resetPosition;//是否要求客户端重置本地预测位置
};

