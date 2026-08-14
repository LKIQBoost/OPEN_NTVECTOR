#include "NtUniSdkBase.h"
#include <cstdio>

// ================================================================
// 静态成员
// ================================================================
NtUniSdkBase* NtUniSdkBase::s_instance = nullptr;

// ================================================================
// 构造 / 析构
// ================================================================
NtUniSdkBase::NtUniSdkBase() {
    s_instance = this;

    // 填满 32 项 noop，防止 SDK 调到越界位置
    for (int i = 0; i < 32; ++i)
        m_delVtbl[i] = (void*)s_del_noop;

    // 已知回调索引
    m_delVtbl[0] = (void*)s_del_onInitFinish;       // onInitFinish
    m_delVtbl[1] = (void*)s_del_onLoginFinish;       // onLoginFinish
    m_delVtbl[2] = (void*)s_del_onLogoutFinish;      // onLogoutFinish
    m_delVtbl[3] = (void*)s_del_onCheckOrder;         // onCheckOrderFinish
    m_delVtbl[4] = (void*)s_del_onExtendFunc;         // onExtendFuncFinish
    m_delVtbl[5] = (void*)s_del_onLog;                // onLog
    m_delVtbl[6] = (void*)s_del_onCompactViewClosed;  // onCompactViewClosed

    m_delegate.vtbl = m_delVtbl;

    // 1. init
    if (!NtUniSDK::SdkMgr::init(nullptr, "3.1.4"))
        return;

    m_sdk.inst = NtUniSDK::SdkMgr::getInst();
    if (!m_sdk.inst) return;
    m_sdk.vt = *(void***)m_sdk.inst;

    // 2. delegate + log hook
    m_sdk.setDelegate(&m_delegate);
    m_sdk.setLogHook(s_logHook);

    // 3. mpay 必填参数
    char title[] = "Login";
    char dataPath[] = "mpay";
    m_sdk.setPtr("mpay_option_login_title", title);
    m_sdk.setPtr("mpay_option_data_path", dataPath);

    // 4. 业务参数
    m_sdk.setStr("JF_GAMEID", "x19");
    m_sdk.setStr("UNISDK_JF_GAS3", "1");
    m_sdk.setStr("JF_LOG_KEY", "3Cz7dGX2EYHORebBUBHwCZ7pltZ_4l-t");
    m_sdk.setStr("JF_PAY_LOG_URL", "https://applog.matrix.netease.com/client/sdk/pay_log");

    // 5. mpay UI 参数
    m_sdk.setStr("MPAY_RESTYPE", "2");
    m_sdk.setStr("MPAY_DEVICE_UID", "device");
    m_sdk.setStr("MPAY_GAME_ICON_PATH", "game-icon");
    m_sdk.setStr("MPAY_ENABLE_PARENT", "0");
    m_sdk.setStr("MPAY_STYLE", "0");
    m_sdk.setStr("MPAY_PAY_ENABLE_PARENT", "0");
    m_sdk.setStr("MPAY_PAY_STYLE", "2");
    m_sdk.setStr("DEBUG_MODE", "0");
    m_sdk.setStr("INNER_COMPACT_LAUNCH_FROM_REMOTE", "1");

    // 6. commitInit
    m_sdk.commitInit();
}

NtUniSdkBase::~NtUniSdkBase() {
    if (m_sdk.ok()) {
        m_sdk.ntCleanup();
        NtUniSDK::SdkMgr::uninit();
    }
    if (s_instance == this)
        s_instance = nullptr;
}

// ================================================================
// 静态回调路由
// ================================================================
void __cdecl NtUniSdkBase::s_logHook(const char* msg) {
    if (s_instance) s_instance->onLog(msg);
}

void __fastcall NtUniSdkBase::s_del_onInitFinish(void* self, int code) {
    if (s_instance) {
        s_instance->m_initDone = true;
        s_instance->onInitFinish(code);
    }
}

void __fastcall NtUniSdkBase::s_del_onLoginFinish(void* self, int code) {
    if (s_instance) {
        s_instance->m_loginCode = code;
        s_instance->m_loggedIn = (code == 0);
        s_instance->onLogin(code);
    }
}

void __fastcall NtUniSdkBase::s_del_onLogoutFinish(void* self, int code) {
    if (s_instance) {
        s_instance->m_loggedIn = false;
        s_instance->onLogout(code);
    }
}

void __fastcall NtUniSdkBase::s_del_onCheckOrder(void* self, void* orderInfo) {
    if (s_instance) s_instance->onCheckOrder(orderInfo);
}

void __fastcall NtUniSdkBase::s_del_onExtendFunc(void* self, void* p) {
    if (s_instance) s_instance->onExtendFunc((const char*)p);
}

void __fastcall NtUniSdkBase::s_del_onLog(void* self, const char* msg) {
    if (s_instance) s_instance->onLog(msg);
}

void __fastcall NtUniSdkBase::s_del_onCompactViewClosed(void* self, int code) {
    if (s_instance) s_instance->onCompactViewClosed(code);
}

void __fastcall NtUniSdkBase::s_del_noop(void* self) {
    // 吞掉未知回调，防崩
}

// ================================================================
// doLogin —— 初始化 + 弹窗 + 消息循环，登录完成后返回
// ================================================================
bool NtUniSdkBase::doLogin() {
    // 7. 父窗口 = 桌面（不用创建窗口，mpay 拿桌面做定位基准）
    HWND desktop = GetDesktopWindow();
    m_sdk.setPtr("mpay_option_parent_hwnd", &desktop);
    // 8. ntLogin
    m_sdk.ntLogin();
    //m_sdk.ntExtend("{\"methodId\":\"updateAgeTipsPosition\",\"anchor\":0,\"xGravity\":0,\"yGravity\":0,\"xOffset\":-9999,\"yOffset\":-9999,\"width\":1,\"height\":1}");

    m_Logout = false;
    // 9. 消息循环：泵消息 + runLoop，直到 onLoginFinish 被回调
    MSG msg;// 适龄提示可能延迟创建，在消息循环里检测一次就够
    bool ageTipHidden = false;

    while (!m_Logout) {
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) return false;
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        m_sdk.ntRunLoop(0.016f);

        if (!ageTipHidden) {
            HWND tip = FindWindowW(L"MPAY_AGE_TIPS", nullptr);
            if (tip) {
                ShowWindow(tip, SW_HIDE);
                ageTipHidden = true;
            }
        }

        Sleep(16);
    }

    return m_loggedIn;
}

// ================================================================
// doLogout
// ================================================================
void NtUniSdkBase::doLogout() {
    if (m_sdk.ok()) m_sdk.ntLogout();
}

// ================================================================
// getter（登录成功后调用）
// ================================================================
const char* NtUniSdkBase::getSauth() const {
    return m_sdk.inst ? ((fn_getPropStr)m_sdk.vt[12])(m_sdk.inst, "SAUTH_JSON") : nullptr;
}
const char* NtUniSdkBase::getAccessToken() const {
    return m_sdk.inst ? ((fn_getPropStr)m_sdk.vt[12])(m_sdk.inst, "UNISDK_JF_ACCESS_TOKEN") : nullptr;
}
const char* NtUniSdkBase::getUid() const {
    return m_sdk.inst ? ((fn_getPropStr)m_sdk.vt[12])(m_sdk.inst, "USERINFO_UID") : nullptr;
}
const char* NtUniSdkBase::getHostId() const {
    return m_sdk.inst ? ((fn_getPropStr)m_sdk.vt[12])(m_sdk.inst, "USERINFO_HOSTID") : nullptr;
}
const char* NtUniSdkBase::getAid() const {
    return m_sdk.inst ? ((fn_getPropStr)m_sdk.vt[12])(m_sdk.inst, "USERINFO_AID") : nullptr;
}
const char* NtUniSdkBase::getLoginJson() const {
    return m_sdk.inst ? ((fn_getPropStr)m_sdk.vt[12])(m_sdk.inst, "UNISDK_LOGIN_JSON") : nullptr;
}
const char* NtUniSdkBase::getStr(const char* key) const {
    return m_sdk.inst ? ((fn_getPropStr)m_sdk.vt[12])(m_sdk.inst, key) : nullptr;
}

// ================================================================
// extendFunc
// ================================================================
void NtUniSdkBase::extendFunc(const char* json) {
    if (m_sdk.ok()) m_sdk.ntExtend(json);
}