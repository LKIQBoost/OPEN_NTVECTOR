// MPayDelegate.h
#pragma once
#include <string>
#include <iostream>
#include <cstdint>

#ifdef _WIN32
  #ifdef MPayDelegate_EXPORTS
    #define MPayDelegate_API __declspec(dllexport)
  #else
    #define MPayDelegate_API __declspec(dllimport)
  #endif
#else
  #define MPayDelegate_API
#endif

// ����SDK�ص��ӿ�
class INtUniSdkDelgate {
public:
    //virtual ~INtUniSdkDelgate() = default;

    // �����͵�����UniSDK�麯����˳����
    // ע�⣺���Ǹ��ݳ���SDK�ƶϵ�˳�򣬿�����Ҫ����

    // [0] ��ʼ�����
    virtual void __cdecl onInitFinish(int code) = 0;

    // [1] ��¼���
    virtual void __cdecl onLoginFinish(int code) = 0;

    // [2] �˳���¼���
    virtual void __cdecl onLogoutFinish(int code) = 0;

    // [3] �û���¼�ɹ�����ϸ��
    virtual void __cdecl onUserLogin(const char* userId, const char* session) = 0;

    // [4] ��¼ʧ��
    virtual void __cdecl onLoginFailed(int errorCode, const char* errorMsg) = 0;

    // [5] ֧�����
    virtual void __cdecl onPayFinish(int code, const char* orderId) = 0;

    // [6] ֧���ɹ�
    virtual void __cdecl onPaySuccess(const char* orderId, const char* productId, int amount) = 0;

    // [7] ֧��ʧ��
    virtual void __cdecl onPayFailed(int errorCode, const char* errorMsg) = 0;

    // [8] ֧��ȡ��
    virtual void __cdecl onPayCanceled() = 0;

    // [9] ��鶩�����
    virtual void __cdecl onCheckOrderFinish(int errorCode, int orderStatus,
        const char* productId, uint32_t productCount,
        const char* orderId, const char* errReason) = 0;

    // [10] ��չ�������
    virtual void __cdecl onExtendFuncFinish(const char* json) = 0;

    // [11] ������ͼ�ر�
    virtual void __cdecl onCompactViewClosed(int code) = 0;

    // [12] ��־�ص�
    virtual void __cdecl onLog(const char* log) = 0;

    // [13] �û��л�
    virtual void __cdecl onUserSwitch(const char* newUserId) = 0;

    // [14] ʵ����֤�ص�
    virtual void __cdecl onRealNameAuth(int status) = 0;

    // [15] �û����Ĺر�
    virtual void __cdecl onUserCenterClosed() = 0;

    // [16] �˳�SDK
    virtual void __cdecl onSDKExit() = 0;

    // [17] ����״̬�仯
    virtual void __cdecl onNetworkStatusChanged(int status) = 0;

    // [18] �û���Ϣ����
    virtual void __cdecl onUserInfoUpdated(const char* json) = 0;

    // [19] ֧����ͼ��ʾ/����
    virtual void __cdecl onPayViewVisible(bool visible) = 0;

    // [20] ��Ʒ��Ϣ����
    virtual void __cdecl onProductInfoUpdated(const char* json) = 0;
};

// �����ʵ����
class MPayDelegate : public INtUniSdkDelgate {
private:
    // ��������˽�г�Ա�����û�����ָ���
    void* m_userData;

public:
    MPayDelegate(void* userData = nullptr);
    //virtual ~MPayDelegate();

    // ʵ�������麯��
    virtual void __cdecl onInitFinish(int code) override;
    virtual void __cdecl onLoginFinish(int code) override;
    virtual void __cdecl onLogoutFinish(int code) override;
    virtual void __cdecl onUserLogin(const char* userId, const char* session) override;
    virtual void __cdecl onLoginFailed(int errorCode, const char* errorMsg) override;
    virtual void __cdecl onPayFinish(int code, const char* orderId) override;
    virtual void __cdecl onPaySuccess(const char* orderId, const char* productId, int amount) override;
    virtual void __cdecl onPayFailed(int errorCode, const char* errorMsg) override;
    virtual void __cdecl onPayCanceled() override;
    virtual void __cdecl onCheckOrderFinish(int errorCode, int orderStatus,
        const char* productId, uint32_t productCount,
        const char* orderId, const char* errReason) override;
    virtual void __cdecl onExtendFuncFinish(const char* json) override;
    virtual void __cdecl onCompactViewClosed(int code) override;
    virtual void __cdecl onLog(const char* log) override;
    virtual void __cdecl onUserSwitch(const char* newUserId) override;
    virtual void __cdecl onRealNameAuth(int status) override;
    virtual void __cdecl onUserCenterClosed() override;
    virtual void __cdecl onSDKExit() override;
    virtual void __cdecl onNetworkStatusChanged(int status) override;
    virtual void __cdecl onUserInfoUpdated(const char* json) override;
    virtual void __cdecl onPayViewVisible(bool visible) override;
    virtual void __cdecl onProductInfoUpdated(const char* json) override;

    // ���ߺ���
    void SetUserData(void* userData);
    void* GetUserData() const;

    // ����ʵ���Ĺ�������
    static MPayDelegate* Create(void* userData = nullptr);
    static void Destroy(MPayDelegate* delegate);
};