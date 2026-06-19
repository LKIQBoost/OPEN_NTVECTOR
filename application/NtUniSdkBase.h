#pragma once

#ifdef _WIN32
#include <windows.h>
#include <string>
#include <atomic>
#include <thread>

namespace NtUniSDK {
    class INtUniSdkGamerInterface;
    class __declspec(dllimport) SdkMgr {
    public:
        static bool init(const char* reserved, const char* version);
        static INtUniSdkGamerInterface* getInst();
        static void uninit();
    };
}

class NtUniSdkBase
{
public:
    NtUniSdkBase();
    virtual ~NtUniSdkBase();

    // ��ʼ������¼������ֱ���û���ɵ�¼��رմ���
    // gameId: ��ϷID���� "x19"��
    // logKey: ��־��Կ
    // version: SDK Э��汾��Ĭ�� "3.1.4"��
    bool doLogin();

    // �ǳ�
    void doLogout();

    // ��ȡ��¼��Ϣ����¼�ɹ�����Ч��
    const char* getSauth()      const;
    const char* getAccessToken() const;
    const char* getUid()         const;
    const char* getHostId()      const;
    const char* getAid()         const;
    const char* getLoginJson()   const;
    const char* getStr(const char* key) const;

    // ��չ��������
    void extendFunc(const char* json);

    // �Ƿ��ѵ�¼
    bool isLoggedIn() const { return m_loggedIn; }

protected:
    // ====== ������д��Щ�ص� ======
    virtual void onInitFinish(int code) {}
    virtual void onLogin(int code) {}
    virtual void onLogout(int code) {}
    virtual void onExtendFunc(const char* json) {}
    virtual void onLog(const char* msg) {}
    virtual void onCheckOrder(void* orderInfo) {}
    virtual void onCompactViewClosed(int code) {}

private:
    // vtable ����ǩ��
    using fn_NTLogHook = void(__fastcall*)(void*, void(__cdecl*)(const char*));
    using fn_setDelegate = void(__fastcall*)(void*, void*);
    using fn_getChannel = const char* (__fastcall*)(void*);
    using fn_getPropStr = const char* (__fastcall*)(void*, const char*);
    using fn_setPropStr = void(__fastcall*)(void*, const char*, const char*);
    using fn_setPropPtr = void(__fastcall*)(void*, const char*, void*);
    using fn_commitInit = void(__fastcall*)(void*);
    using fn_ntLogin = void(__fastcall*)(void*);
    using fn_ntLogout = void(__fastcall*)(void*);
    using fn_ntExtendFunc = void(__fastcall*)(void*, const char*);
    using fn_ntRunLoop = void(__fastcall*)(void*, float);
    using fn_ntCleanup = void(__fastcall*)(void*);

    // SDK �ڲ����÷�װ
    struct Sdk {
        void* inst = nullptr;
        void** vt = nullptr;
        bool ok() const { return inst && vt; }

        void        setLogHook(void(__cdecl* f)(const char*)) { ((fn_NTLogHook)vt[0])(inst, f); }
        void        setDelegate(void* d) { ((fn_setDelegate)vt[2])(inst, d); }
        const char* getChannel() { return ((fn_getChannel)vt[5])(inst); }
        const char* getStr(const char* k) { return ((fn_getPropStr)vt[12])(inst, k); }
        void        setStr(const char* k, const char* v) { ((fn_setPropStr)vt[13])(inst, k, v); }
        void        setPtr(const char* k, void* v) { ((fn_setPropPtr)vt[29])(inst, k, v); }
        void        commitInit() { ((fn_commitInit)vt[14])(inst); }
        void        ntLogin() { ((fn_ntLogin)vt[15])(inst); }
        void        ntLogout() { ((fn_ntLogout)vt[17])(inst); }
        void        ntExtend(const char* json) { ((fn_ntExtendFunc)vt[19])(inst, json); }
        void        ntRunLoop(float dt) { ((fn_ntRunLoop)vt[20])(inst, dt); }
        void        ntCleanup() { ((fn_ntCleanup)vt[27])(inst); }
    };

    Sdk m_sdk;

    std::atomic<bool> m_initDone{ false };
    std::atomic<bool> m_loggedIn{ false };
    std::atomic<bool> m_Logout{ false };
    std::atomic<int>  m_loginCode{ -1 };

    // delegate �����32 ���Խ�磩
    void* m_delVtbl[32];
    struct FakeDelegate { void** vtbl; };
    FakeDelegate m_delegate;

    // ȫ����־���ӻص���ֻ���Ǿ�̬�ģ�SDK �ӿ����ƣ�
    static NtUniSdkBase* s_instance;
    static void __cdecl s_logHook(const char* msg);

    // delegate �ص�·�ɣ���̬ �� ʵ���麯����
    static void __fastcall s_del_onInitFinish(void* self, int code);
    static void __fastcall s_del_onLoginFinish(void* self, int code);
    static void __fastcall s_del_onLogoutFinish(void* self, int code);
    static void __fastcall s_del_onCheckOrder(void* self, void* orderInfo);
    static void __fastcall s_del_onExtendFunc(void* self, void* p);
    static void __fastcall s_del_onLog(void* self, const char* msg);
    static void __fastcall s_del_onCompactViewClosed(void* self, int code);
    static void __fastcall s_del_noop(void* self);
};

#endif // _WIN32