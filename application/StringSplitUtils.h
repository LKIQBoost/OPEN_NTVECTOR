#pragma once
#include <string>
#include <sstream>
#include <iomanip>
#include <cstdint>
class StringSplitUtils
{
public:
    static std::string escape_backslashes(const std::string& input) {
        std::string result;

        for (char c : input) {
            if (c == '\\') {
                result += "\\\\";  // ������б�ܾ���������
            }
            else {
                result += c;       // �����ַ�ֱ������
            }
        }

        return result;
    }
    template<typename T>
    static std::string pointerToHexString(T* ptr) {
        std::stringstream ss;
        ss << "0x" << std::uppercase << std::setfill('0') << std::setw(16)
            << std::hex << reinterpret_cast<uintptr_t>(ptr);
        return ss.str();
    }

    static std::string ToFileName(std::string path) {
        // һ�д�����ȡ
        std::string name = path.substr(path.find_last_of("/\\") + 1);
        name = name.substr(0, name.find_last_of('.'));

        return name;
    }
};

