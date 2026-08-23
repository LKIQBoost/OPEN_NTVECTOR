#pragma once

#ifdef _WIN32
#include "NtUniSdkBase.h"
#include "engine_wrapper.h"
#include "Logger.h"
class MPayWrapper : public NtUniSdkBase
{
protected:
    void onLogin(int code) override {
        PythonEventEngine e;
        if (code == 0) {
            std::string sauth_json = getSauth();
            e.trigger("unisdk_start_up", code ,sauth_json);
        }
        else {
            Logger::getInstance().logv(LOG_ERROR, "Login result is %d", code);
            e.trigger("unisdk_start_up", code);
        }
    }
};
#endif // _WIN32

