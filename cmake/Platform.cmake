# Platform.cmake - 平台检测与编译器选项
#
# 支持平台:
#   - Windows (MSVC)
#   - Linux x86_64 (GCC / Clang)
#   - Android ARM64 (Termux 原生编译, Clang)
#
# Android/Termux 注意事项:
#   1. Bionic libc 缺少 libutil/librt，不能链接 -lutil -lrt -lpthread
#   2. Python 2.7 configure 需要 config.site 禁用 Bionic 不存在的函数
#      (getloadavg, setpwent, getpwent, forkpty, openpty 等)
#   3. Clang 21+ 的 libc++_shared.so 缺少 __cxa_init_primary_exception 等符号
#      由 application/platform/cxa_stub.cpp 提供弱符号 stub
#   4. 文件系统大小写敏感，SLikeNet 目录名必须与 #include 一致
#   5. Python 2.7 必须用 -std=gnu11 编译 (c11 隐藏 POSIX 声明)

# 通用编译定义
add_compile_definitions(OPENSSL_STATIC)

# Android / Termux 检测
# 可通过 cmake -DPLATFORM_ANDROID=ON 手动指定
# Termux 原生编译时 CMAKE_SYSTEM_NAME=Linux 但 Bionic libc 缺少 libutil 等
if(NOT WIN32 AND NOT DEFINED PLATFORM_ANDROID)
    if(ANDROID OR DEFINED ENV{ANDROID_ROOT} OR EXISTS "/system/bin/linker64")
        set(PLATFORM_ANDROID TRUE)
    else()
        set(PLATFORM_ANDROID FALSE)
    endif()
endif()
if(PLATFORM_ANDROID)
    message(STATUS "Platform: Android/Termux (Bionic libc, no libutil/librt)")
endif()

if(MSVC)
    add_compile_options(/MP)
    add_compile_definitions(
        _CRT_SECURE_NO_WARNINGS
        _WINSOCK_DEPRECATED_NO_WARNINGS
        _CONSOLE
        RTC_STATIC
        WIN32
    )
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL" CACHE STRING "MSVC runtime")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    add_compile_options(-Wall -fPIC)
    if(NOT PLATFORM_ANDROID)
        # Android Bionic 内置 pthread，不需要 -pthread 标志
        add_compile_options(-pthread)
    endif()
endif()
