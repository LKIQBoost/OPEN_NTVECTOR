#pragma once
// 数据包 ID 宏定义
// https://prismarinejs.github.io/minecraft-data/protocol/bedrock/1.21.42/#Action
#include <vector>
#include <string>
#include "BinaryWriter.h"
#include "BinaryReader.h"


#define RakNetID 0xfe

#define IDLogin 1                  // 客户端登录（本地）(发送chain交换密钥)
#define IDPlayStatus 2             // 玩家状态
#define IDServerToClientHandshake 3 // 服务端到客户端握手(携带服务器公钥和salt(值))
#define IDClientToServerHandshake 4 // 客户端到服务端握手(空值数据包)
#define IDDisconnect 5             // 断开连接
#define IDResourcePacksInfo 6      // 资源包信息（非常用）
#define IDResourcePackStack 7      // 资源包堆叠（非常用）
#define IDResourcePackClientResponse 8 // 资源包客户端响应（非常用）
#define IDText 9                   // 文本消息
#define IDSetTime 10               // 更新客户端时间（服务端 -> 客户端）
#define IDStartGame 11             // 开始游戏
#define IDAddPlayer 12             // 添加玩家实体
#define IDAddActor 13              // 添加实体
#define IDRemoveActor 14           // 添加实体
#define IDAddItemActor 15          // 添加物品实体
#define IDTakeItemActor 17         // 捡起物品实体（动画）
#define IDMoveActorAbsolute 18     // 移动实体到绝对位置
#define IDMovePlayer 19            // 玩家移动（服务端 <-> 客户端）
#define IDPassengerJump 20         // 乘骑跳跃（客户端 -> 服务端）
#define IDUpdateBlock 21           // 更新方块（单方块修改）
#define IDAddPainting 22           // 添加绘画实体
#define IDTickSync 23              // 同步Tick（服务端 <-> 客户端）
#define IDLevelSoundEventV1 24     // ???
#define IDLevelEvent 25            // 世界事件（服务端 -> 客户端）
#define IDBlockEvent 26            // 方块事件（服务端 -> 客户端）[打开箱子./././...]
#define IDActorEvent 27            // 实体事件（服务端 -> 客户端）[狼抖干自己./././...]
#define IDMobEffect 28             // 生物效果（服务端 -> 客户端）
#define IDUpdateAttributes 29      // 更新实体属性（服务端 -> 客户端）[移动速度./健康状况./...]
#define IDInventoryTransaction 30  // 物品交易（服务端 <- 客户端）
#define IDMobEquipment 31          // 实体物品持有（服务端 <-> 客户端）[僵尸手持石剑././...]
#define IDMobArmourEquipment 32    // 装备穿戴（服务端 -> 客户端）[玩家./僵尸./其他实体./...]
#define IDInteract 33              // 实体交互（弃用）
#define IDBlockPickRequest 34      // 拾取物品请求（客户端 -> 服务端）
#define IDActorPickRequest 35      // 拾取实体请求（客户端 -> 服务端）
#define IDPlayerAction 36          // 玩家行为（客户端 -> 服务端）
#define IDHurtArmour 38            // 盔甲损害（服务端 -> 客户端）
#define IDSetActorData 39          // 实体元数据（服务端 -> 客户端）[实体是否着火././...]
#define IDSetActorMotion 40        // 设置客户端速度（服务端 -> 客户端）
#define IDSetActorLink 41          // 设置实体乘骑（服务端 -> 客户端）
#define IDSetHealth 42             // 设置玩家血量（服务端 -> 客户端）
#define IDSetSpawnPosition 43      // 设置玩家出生点位置（服务端 -> 客户端）
#define IDAnimate 44               // 动画效果（服务端 -> 客户端）
#define IDRespawn 45               // 重生（服务端 <-> 客户端）
#define IDContainerOpen 46         // 打开容器（服务端 -> 客户端）
#define IDContainerClose 47        // 关闭容器（服务端 -> 客户端）
#define IDPlayerHotBar 48          // 玩家快捷栏槽位（服务端 -> 客户端）
#define IDInventoryContent 49      // 更新玩家背包（服务端 -> 客户端）
#define IDInventorySlot 50         // 玩家背包单槽位更新（服务端 -> 客户端）
#define IDContainerSetData 51      // 容器设置数据（服务端 -> 客户端）
#define IDCraftingData 52          // 合成数据（服务端 -> 客户端）
#define IDCraftingEvent 53         // 合成事件（客户端 -> 服务端）
#define IDGUIDataPickItem 54       // GUI数据拾取物品（客户端 -> 服务端）
#define IDAdventureSettings 55     // 冒险设置（服务端 -> 客户端）
#define IDBlockActorData 56        // 方块实体数据（服务端 -> 客户端）
#define IDPlayerInput 57           // 玩家输入（客户端 -> 服务端）
#define IDLevelChunk 58            // 区块数据（服务端 -> 客户端）
#define IDSetCommandsEnabled 59    // 设置命令启用（服务端 -> 客户端）
#define IDSetDifficulty 60         // 设置难度（服务端 -> 客户端）
#define IDChangeDimension 61       // 改变维度（服务端 -> 客户端）
#define IDSetPlayerGameType 62     // 设置玩家游戏类型（服务端 -> 客户端）
#define IDPlayerList 63            // 玩家列表（服务端 -> 客户端）
#define IDSimpleEvent 64           // 简单事件（服务端 -> 客户端）
#define IDEvent 65                 // 事件（服务端 -> 客户端）
#define IDSpawnExperienceOrb 66    // 生成经验球（服务端 -> 客户端）
#define IDClientBoundMapItemData 67 // 客户端绑定地图物品数据（服务端 -> 客户端）
#define IDMapInfoRequest 68        // 地图信息请求（客户端 -> 服务端）
#define IDRequestChunkRadius 69    // 请求区块半径（客户端 -> 服务端）
#define IDChunkRadiusUpdated 70    // 区块半径更新（服务端 -> 客户端）
#define IDItemFrameDropItem 71     // 物品展示框掉落物品（服务端 -> 客户端）
#define IDGameRulesChanged 72      // 游戏规则改变（服务端 -> 客户端）
#define IDCamera 73                // 相机（服务端 -> 客户端）
#define IDBossEvent 74             // Boss事件（服务端 -> 客户端）
#define IDShowCredits 75           // 显示 credits（服务端 -> 客户端）
#define IDAvailableCommands 76     // 可用命令（服务端 -> 客户端）
#define IDCommandRequest 77        // 命令请求（客户端 -> 服务端）
#define IDCommandBlockUpdate 78    // 命令块更新（客户端 -> 服务端）
#define IDCommandOutput 79         // 命令输出（服务端 -> 客户端）
#define IDUpdateTrade 80           // 更新交易（服务端 -> 客户端）
#define IDUpdateEquip 81           // 更新装备（服务端 -> 客户端）
#define IDResourcePackDataInfo 82  // 资源包数据信息（服务端 -> 客户端）
#define IDResourcePackChunkData 83 // 资源包块数据（服务端 -> 客户端）
#define IDResourcePackChunkRequest 84 // 资源包块请求（客户端 -> 服务端）
#define IDTransfer 85              // 传输（服务端 -> 客户端）
#define IDPlaySound 86             // 播放声音（服务端 -> 客户端）
#define IDStopSound 87             // 停止声音（服务端 -> 客户端）
#define IDSetTitle 88              // 设置标题（服务端 -> 客户端）
#define IDAddBehaviourTree 89      // 添加行为树（服务端 -> 客户端）
#define IDStructureBlockUpdate 90  // 结构方块更新（客户端 -> 服务端）
#define IDShowStoreOffer 91        // 显示商店优惠（服务端 -> 客户端）
#define IDPurchaseReceipt 92       // 购买收据（客户端 -> 服务端）
#define IDPlayerSkin 93            // 玩家皮肤（客户端 -> 服务端）
#define IDSubClientLogin 94        // 子客户端登录（客户端 -> 服务端）
#define IDAutomationClientConnect 95 // 自动化客户端连接（客户端 -> 服务端）
#define IDSetLastHurtBy 96         // 设置最后受伤来源（服务端 -> 客户端）
#define IDBookEdit 97              // 书本编辑（客户端 -> 服务端）
#define IDNPCRequest 98            // NPC请求（客户端 -> 服务端）
#define IDPhotoTransfer 99         // 照片传输（客户端 -> 服务端）
#define IDModalFormRequest 100     // 模态表单请求（服务端 -> 客户端）
#define IDModalFormResponse 101    // 模态表单响应（客户端 -> 服务端）
#define IDServerSettingsRequest 102 // 服务器设置请求（客户端 -> 服务端）
#define IDServerSettingsResponse 103 // 服务器设置响应（服务端 -> 客户端）
#define IDShowProfile 104          // 显示资料（客户端 -> 服务端）
#define IDSetDefaultGameType 105   // 设置默认游戏类型（服务端 -> 客户端）
#define IDRemoveObjective 106      // 移除目标（服务端 -> 客户端）
#define IDSetDisplayObjective 107  // 设置显示目标（服务端 -> 客户端）
#define IDSetScore 108             // 设置分数（服务端 -> 客户端）
#define IDLabTable 109             // 实验室表（服务端 -> 客户端）
#define IDUpdateBlockSynced 110    // 同步更新方块（服务端 -> 客户端）
#define IDMoveActorDelta 111       // 移动实体增量（服务端 -> 客户端）
#define IDSetScoreboardIdentity 112 // 设置计分板身份（服务端 -> 客户端）
#define IDSetLocalPlayerAsInitialised 113 // 设置本地玩家为已初始化（服务端 -> 客户端）
#define IDUpdateSoftEnum 114       // 更新软枚举（服务端 -> 客户端）
#define IDNetworkStackLatency 115  // 网络堆栈延迟（服务端 -> 客户端）
#define IDScriptCustomEvent 117    // 脚本自定义事件（客户端 -> 服务端）
#define IDSpawnParticleEffect 118  // 生成粒子效果（服务端 -> 客户端）
#define IDAvailableActorIdentifiers 119 // 可用实体标识符（服务端 -> 客户端）
#define IDLevelSoundEventV2 120    // 世界声音事件 V2（服务端 -> 客户端）
#define IDNetworkChunkPublisherUpdate 121 // 网络区块发布者更新（服务端 -> 客户端）
#define IDBiomeDefinitionList 122  // 生物群系定义列表（服务端 -> 客户端）
#define IDLevelSoundEvent 123      // 世界声音事件（服务端 -> 客户端）
#define IDLevelEventGeneric 124    // 世界事件通用（服务端 -> 客户端）
#define IDLecternUpdate 125        // 讲台更新（客户端 -> 服务端）
#define IDAddEntity 127            // 添加实体（服务端 -> 客户端）
#define IDRemoveEntity 128         // 移除实体（服务端 -> 客户端）
#define IDClientCacheStatus 129    // 客户端缓存状态（客户端 -> 服务端）
#define IDOnScreenTextureAnimation 130 // 屏幕纹理动画（服务端 -> 客户端）
#define IDMapCreateLockedCopy 131  // 地图创建锁定副本（服务端 -> 客户端）
#define IDStructureTemplateDataRequest 132 // 结构模板数据请求（客户端 -> 服务端）
#define IDStructureTemplateDataResponse 133 // 结构模板数据响应（服务端 -> 客户端）
#define IDClientCacheBlobStatus 135 // 客户端缓存块状态（客户端 -> 服务端）
#define IDClientCacheMissResponse 136 // 客户端缓存未命中响应（服务端 -> 客户端）
#define IDEducationSettings 137    // 教育设置（服务端 -> 客户端）
#define IDEmote 138                // 表情（客户端 -> 服务端）
#define IDMultiPlayerSettings 139  // 多人游戏设置（客户端 -> 服务端）
#define IDSettingsCommand 140      // 设置命令（客户端 -> 服务端）
#define IDAnvilDamage 141          // 铁砧损坏（服务端 -> 客户端）
#define IDCompletedUsingItem 142   // 完成使用物品（客户端 -> 服务端）
#define IDNetworkSettings 143      // 网络设置（服务端 -> 客户端）
#define IDPlayerAuthInput 144      // 玩家认证输入（客户端 -> 服务端）
#define IDCreativeContent 145      // 创造内容（服务端 -> 客户端）
#define IDPlayerEnchantOptions 146 // 玩家附魔选项（服务端 -> 客户端）
#define IDItemStackRequest 147     // 物品堆栈请求（客户端 -> 服务端）
#define IDItemStackResponse 148    // 物品堆栈响应（服务端 -> 客户端）
#define IDPlayerArmourDamage 149   // 玩家盔甲损坏（服务端 -> 客户端）
#define IDCodeBuilder 150          // 代码构建器（客户端 -> 服务端）
#define IDUpdatePlayerGameType 151 // 更新玩家游戏类型（服务端 -> 客户端）
#define IDEmoteList 152            // 表情列表（服务端 -> 客户端）
#define IDPositionTrackingDBServerBroadcast 153 // 位置跟踪数据库服务器广播（服务端 -> 客户端）
#define IDPositionTrackingDBClientRequest 154 // 位置跟踪数据库客户端请求（客户端 -> 服务端）
#define IDDebugInfo 155            // 调试信息（服务端 -> 客户端）
#define IDPacketViolationWarning 156 // 数据包违规警告（服务端 -> 客户端）
#define IDMotionPredictionHints 157 // 运动预测提示（服务端 -> 客户端）
#define IDAnimateEntity 158        // 动画实体（服务端 -> 客户端）
#define IDCameraShake 159          // 相机震动（服务端 -> 客户端）
#define IDPlayerFog 160            // 玩家迷雾（服务端 -> 客户端）
#define IDCorrectPlayerMovePrediction 161 // 纠正玩家移动预测（服务端 -> 客户端）
#define IDItemComponent 162        // 物品组件（服务端 -> 客户端）
#define IDFilterText 163           // 过滤文本（客户端 -> 服务端）
#define IDClientBoundDebugRenderer 164 // 客户端绑定调试渲染器（服务端 -> 客户端）
#define IDSyncActorProperty 165    // 同步实体属性（服务端 -> 客户端）
#define IDAddVolumeEntity 166      // 添加体积实体（服务端 -> 客户端）
#define IDRemoveVolumeEntity 167   // 移除体积实体（服务端 -> 客户端）
#define IDSimulationType 168       // 模拟类型（服务端 -> 客户端）
#define IDNPCDialogue 169          // NPC对话（服务端 -> 客户端）
#define IDEducationResourceURI 170 // 教育资源URI（服务端 -> 客户端）
#define IDCreatePhoto 171          // 创建照片（服务端 -> 客户端）
#define IDUpdateSubChunkBlocks 172 // 更新子区块方块（服务端 -> 客户端）
#define IDPhotoInfoRequest 173     // 照片信息请求（客户端 -> 服务端）
#define IDSubChunk 174             // 子区块（服务端 -> 客户端）
#define IDSubChunkRequest 175      // 子区块请求（客户端 -> 服务端）
#define IDClientStartItemCooldown 176 // 客户端开始物品冷却（客户端 -> 服务端）
#define IDScriptMessage 177        // 脚本消息（客户端 -> 服务端）
#define IDCodeBuilderSource 178    // 代码构建器源（客户端 -> 服务端）
#define IDTickingAreasLoadStatus 179 // ticking 区域加载状态（服务端 -> 客户端）
#define IDDimensionData 180        // 维度数据（服务端 -> 客户端）
#define IDAgentAction 181          // 代理
#define IDChangeMobProperty 182    // 改变生物属性（服务端 -> 客户端）
#define IDLessonProgress 183       // 课程进度（客户端 -> 服务端）
#define IDRequestAbility 184       // 请求能力（客户端 -> 服务端）
#define IDRequestPermissions 185   // 请求权限（客户端 -> 服务端）
#define IDToastRequest 186         // 吐司请求（服务端 -> 客户端）
#define IDUpdateAbilities 187      // 更新能力（服务端 -> 客户端）
#define IDUpdateAdventureSettings 188 // 更新冒险设置（服务端 -> 客户端）
#define IDDeathInfo 189            // 死亡信息（服务端 -> 客户端）
#define IDEditorNetwork 190        // 编辑器网络（服务端 -> 客户端）
#define IDFeatureRegistry 191      // 特性注册表（服务端 -> 客户端）
#define IDServerStats 192          // 服务器统计（服务端 -> 客户端）
#define IDRequestNetworkSettings 193 // 请求网络设置（客户端 -> 服务端）
#define IDGameTestRequest 194      // 游戏测试请求（客户端 -> 服务端）
#define IDGameTestResults 195      // 游戏测试结果（服务端 -> 客户端）
#define IDUpdateClientInputLocks 196 // 更新客户端输入锁（服务端 -> 客户端）
#define IDClientCheatAbility 197   // 客户端作弊能力（客户端 -> 服务端）
#define IDCameraPresets 198        // 相机预设（服务端 -> 客户端）
#define IDUnlockedRecipes 199      // 已解锁配方（服务端 -> 客户端）
#define IDPyRpc 200                // Python远程过程调用（客户端 -> 服务端）
#define IDChangeModel 201          // 改变模型（服务端 -> 客户端）
#define IDStoreBuySucc 202         // 商店购买成功（服务端 -> 客户端）
#define IDNeteaseJson 203          // 网易 JSON（客户端 -> 服务端）
#define IDChangeModelTexture 204   // 改变模型纹理（服务端 -> 客户端）
#define IDChangeModelOffset 205    // 改变模型偏移（服务端 -> 客户端）
#define IDChangeModelBind 206      // 改变模型绑定（服务端 -> 客户端）
#define IDHungerAttr 207           // 饥饿属性（服务端 -> 客户端）
#define IDSetDimensionLocalTime 208 // 设置维度本地时间（服务端 -> 客户端）
#define IDWithdrawFurnaceXp 209    // 提取熔炉经验（客户端 -> 服务端）
#define IDSetDimensionLocalWeather 210 // 设置维度本地天气（服务端 -> 客户端）
#define IDCustomV1 223             // 自定义 V1（服务端 -> 客户端）
#define IDCombine 224              // 组合（服务端 -> 客户端）
#define IDVConnection 225          // V 连接（服务端 -> 客户端）
#define IDTransport 226            // 传输（服务端 -> 客户端）
#define IDCustomV2 227             // 自定义 V2（服务端 -> 客户端）
#define IDConfirmSkin 228          // 确认皮肤（服务端 -> 客户端）
#define IDTransportNoCompress 229  // 无压缩传输（服务端 -> 客户端）
#define IDMobEffectV2 230          // 生物效果 V2（服务端 -> 客户端）
#define IDMobBlockActorChanged 231 // 生物方块实体改变（服务端 -> 客户端）
#define IDChangeActorMotion 232    // 改变实体运动（服务端 -> 客户端）
#define IDAnimateEmoteEntity 233   // 动画表情实体（服务端 -> 客户端）
#define IDCameraInstruction 300    // 相机指令（服务端 -> 客户端）
#define IDCompressedBiomeDefinitionList 301 // 压缩生物群系定义列表（服务端 -> 客户端）
#define IDTrimData 302             // 修剪数据（服务端 -> 客户端）
#define IDOpenSign 303             // 打开标志（服务端 -> 客户端）
#define IDAgentAnimation 304       // 代理动画（服务端 -> 客户端）


#define PyRpcClientID 0x05db23ae
#define PyRpcServerID 0x0094d408

class PacketBase
{
public:
	virtual unsigned char ID();
	virtual std::vector<unsigned char> Serializ();
	virtual void Deserializ(std::vector<unsigned char> pack);
	static size_t calculateVarintSizeFast(int32_t value) {
		uint32_t v = static_cast<uint32_t>(value < 0 ? ~value + 1 : value);
		if (v < (1 << 7)) return 1;
		if (v < (1 << 14)) return 2;
		if (v < (1 << 21)) return 3;
		if (v < (1 << 28)) return 4;
		return 5;  // int32_t 最多 5 字节（含符号位）
	}

};
/*
class BehaviourPackInfo {
	// UUID is the UUID of the behaviour pack. Each behaviour pack downloaded must have a different UUID in
	// order for the client to be able to handle them properly.
	string UUID;
	// Version is the version of the behaviour pack. The client will cache behaviour packs sent by the server as
	// long as they carry the same version. Sending a behaviour pack with a different version than previously
	// will force the client to re-download it.
	string	Version;
	// Size is the total size in bytes that the behaviour pack occupies. This is the size of the compressed
	// archive (zip) of the behaviour pack.
	uint64_t Size;
	// ContentKey is the key used to decrypt the behaviour pack if it is encrypted. This is generally the case
	// for marketplace behaviour packs.
	string	ContentKey;
	// SubPackName ...
	string	SubPackName;
	// ContentIdentity ...
	string	ContentIdentity;
	// HasScripts specifies if the behaviour packs has any scripts in it. A client will only download the
	// behaviour pack if it supports scripts, which, up to 1.11, only includes Windows 10.
	bool	HasScripts;
};*/

using namespace std;

