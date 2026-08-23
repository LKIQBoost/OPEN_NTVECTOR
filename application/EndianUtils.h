#pragma once
#include <algorithm> // for std::move
#include <cstring>   // for memcpy
#include <cstddef> // for size_t
#include <cstdint> // for uint8_t
class EndianUtils
{
public:
    template <typename T>
    static T SwapEndian(T value) {
        union {
            T val;
            uint8_t bytes[sizeof(T)];
        } src, dst;

        src.val = value;
        std::reverse_copy(src.bytes, src.bytes + sizeof(T), dst.bytes);
        return dst.val;
    }

    template <typename T>
    static T BigToLittleEndian(T value) {
        return EndianUtils::SwapEndian(value);
    }

    template <typename T>
    static T LittleToBigEndian(T value) {
        return EndianUtils::SwapEndian(value);
    }
};

