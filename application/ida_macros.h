#pragma once
#include <stdint.h>

// 基本类型定义
/*
typedef uint8_t  _BYTE;
typedef uint16_t _WORD;
typedef uint32_t _DWORD;
typedef uint64_t _QWORD;
typedef int8_t   __int8;
typedef int16_t  __int16;
typedef int32_t  __int32;
typedef int64_t  __int64;
typedef uint8_t  unsigned __int8;
typedef uint16_t unsigned __int16;
typedef uint32_t unsigned __int32;
typedef uint64_t unsigned __int64;*/
// 循环移位宏
#define __ROL4__(x, n) (((x) << (n)) | ((x) >> (32 - (n))))
#define __ROR4__(x, n) (((x) >> (n)) | ((x) << (32 - (n))))
#define __ROL8__(x, n) (((x) << (n)) | ((x) >> (64 - (n))))
#define __ROR8__(x, n) (((x) >> (n)) | ((x) << (64 - (n))))

// 对于固定位移数的情况可以定义特定版本
#define __ROL4_1__(x)  (((x) << 1) | ((x) >> 31))
#define __ROR4_1__(x)  (((x) >> 1) | ((x) << 31))
// 64位值的高低位分离
#define LODWORD(x) ((uint32_t)((x) & 0xFFFFFFFF))
#define HIDWORD(x) ((uint32_t)(((x) >> 32) & 0xFFFFFFFF))

// 32位值的高低位分离
#define LOWORD(x)  ((uint16_t)((x) & 0xFFFF))
#define HIWORD(x)  ((uint16_t)(((x) >> 16) & 0xFFFF))

// 16位值的高低位分离
#define LOBYTE(x)  ((uint8_t)((x) & 0xFF))
#define HIBYTE(x)  ((uint8_t)(((x) >> 8) & 0xFF))
// 组合宏
#define __PAIR64__(hi, lo) (((uint64_t)(hi) << 32) | (uint32_t)(lo))
#define MAKEWORD(lo, hi)   ((uint16_t)(((uint8_t)(lo)) | ((uint16_t)((uint8_t)(hi))) << 8))
#define MAKEDWORD(lo, hi)  ((uint32_t)(((uint16_t)(lo)) | ((uint32_t)((uint16_t)(hi))) << 16))
// 调用约定宏(根据编译器不同)
#ifdef _MSC_VER
#define __cdecl    __cdecl
#define __stdcall  __stdcall
#define __fastcall __fastcall
#define __thiscall __thiscall
#else
    // GCC/Clang等编译器
#define __cdecl    __attribute__((cdecl))
#define __stdcall  __attribute__((stdcall))
#define __fastcall __attribute__((fastcall))
#define __thiscall // 通常GCC不支持thiscall
#endif
// 安全转换宏
#define SAFE_CAST(type, expr) ((type)(expr))

// 带符号扩展的转换
#define SIGN_EXTEND_32(x, bits) (((int32_t)((x) << (32 - (bits)))) >> (32 - (bits)))
#define SIGN_EXTEND_64(x, bits) (((int64_t)((x) << (64 - (bits)))) >> (64 - (bits)))