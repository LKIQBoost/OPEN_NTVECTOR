// MPayDelegate.cpp
#include "MPayDelegate.h"
#include <chrono>
#include <iomanip>
#include <sstream>

MPayDelegate::MPayDelegate(void* userData)
    : m_userData(userData) {
    //std::cout << "[MPayDelegate] Constructor called" << std::endl;
}

//MPayDelegate::~MPayDelegate() {
//    std::cout << "[MPayDelegate] Destructor called" << std::endl;
//}


// 只实现最基本的方法
void __cdecl  MPayDelegate::onInitFinish(int code) {
}

void __cdecl  MPayDelegate::onLoginFinish(int code) {
}

// [2] 退出登录完成
void __cdecl MPayDelegate::onLogoutFinish(int code) {
}

// [3] 用户登录成功（详细）
void __cdecl MPayDelegate::onUserLogin(const char* userId, const char* session) {
    // 这里可以保存用户会话信息
}

// [4] 登录失败
void __cdecl MPayDelegate::onLoginFailed(int errorCode, const char* errorMsg) {
}

// [5] 支付完成
void __cdecl MPayDelegate::onPayFinish(int code, const char* orderId) {
}

// [6] 支付成功
void __cdecl MPayDelegate::onPaySuccess(const char* orderId, const char* productId, int amount) {
    // 这里应该处理支付成功逻辑，如发货等
}

// [7] 支付失败
void __cdecl MPayDelegate::onPayFailed(int errorCode, const char* errorMsg) {
}

// [8] 支付取消
void __cdecl MPayDelegate::onPayCanceled() {
}

// [9] 检查订单完成
void __cdecl MPayDelegate::onCheckOrderFinish(int errorCode, int orderStatus,
    const char* productId, uint32_t productCount,
    const char* orderId, const char* errReason) {
}

// [10] 扩展功能完成
void __cdecl MPayDelegate::onExtendFuncFinish(const char* json) {
    // json可能包含扩展功能的结果数据
}

// [11] 紧凑视图关闭
void __cdecl MPayDelegate::onCompactViewClosed(int code) {
    // code: 0=正常关闭, 其他=错误码
}

// [12] 日志回调
void __cdecl MPayDelegate::onLog(const char* log) {
    // 注意：这个回调可能很频繁，谨慎输出
    std::cout << log << std::endl;
}

// [13] 用户切换
void __cdecl MPayDelegate::onUserSwitch(const char* newUserId) {
}

// [14] 实名认证回调
void __cdecl MPayDelegate::onRealNameAuth(int status) {
}

// [15] 用户中心关闭
void __cdecl MPayDelegate::onUserCenterClosed() {
}

// [16] 退出SDK
void __cdecl MPayDelegate::onSDKExit() {
}

// [17] 网络状态变化
void __cdecl MPayDelegate::onNetworkStatusChanged(int status) {
}

// [18] 用户信息更新
void __cdecl MPayDelegate::onUserInfoUpdated(const char* json) {
    // json包含用户信息，如昵称、头像等
}

// [19] 支付视图显示/隐藏
void __cdecl MPayDelegate::onPayViewVisible(bool visible) {
}

// [20] 商品信息更新
void __cdecl MPayDelegate::onProductInfoUpdated(const char* json) {
    // json包含商品信息列表
}

// 工具函数实现
void MPayDelegate::SetUserData(void* userData) {
    m_userData = userData;
}

void* MPayDelegate::GetUserData() const {
    return m_userData;
}

MPayDelegate* MPayDelegate::Create(void* userData) {
    return new MPayDelegate(userData);
}

void __cdecl MPayDelegate::Destroy(MPayDelegate* delegate) {
    delete delegate;
}