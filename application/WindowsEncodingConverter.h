#pragma once
#include <iostream>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

class WindowsEncodingConverter {
public:
    static std::string gbkToUtf8(const std::string& gbkStr) {
#ifdef _WIN32
        int wlen = MultiByteToWideChar(CP_ACP, 0, gbkStr.c_str(), -1, nullptr, 0);
        if (wlen == 0) return gbkStr;

        wchar_t* wstr = new wchar_t[wlen];
        MultiByteToWideChar(CP_ACP, 0, gbkStr.c_str(), -1, wstr, wlen);

        int ulen = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, nullptr, 0, nullptr, nullptr);
        if (ulen == 0) {
            delete[] wstr;
            return gbkStr;
        }

        char* utf8str = new char[ulen];
        WideCharToMultiByte(CP_UTF8, 0, wstr, -1, utf8str, ulen, nullptr, nullptr);

        std::string result(utf8str);
        delete[] wstr;
        delete[] utf8str;
        return result;
#else
        return gbkStr;
#endif
    }

    static std::string utf8ToGbk(const std::string& utf8Str) {
#ifdef _WIN32
        int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, nullptr, 0);
        if (wlen == 0) return utf8Str;

        wchar_t* wstr = new wchar_t[wlen];
        MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, wstr, wlen);

        int glen = WideCharToMultiByte(CP_ACP, 0, wstr, -1, nullptr, 0, nullptr, nullptr);
        if (glen == 0) {
            delete[] wstr;
            return utf8Str;
        }

        char* gbkstr = new char[glen];
        WideCharToMultiByte(CP_ACP, 0, wstr, -1, gbkstr, glen, nullptr, nullptr);

        std::string result(gbkstr);
        delete[] wstr;
        delete[] gbkstr;
        return result;
#else
        return utf8Str;
#endif
    }
};

