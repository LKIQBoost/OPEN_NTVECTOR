# Platform.cmake - 平台检测与编译器选项

# 通用编译定义
add_compile_definitions(OPENSSL_STATIC)

if(MSVC)
    # MSVC 特定选项
    add_compile_options(/MP)
    add_compile_definitions(
        _CRT_SECURE_NO_WARNINGS
        _WINSOCK_DEPRECATED_NO_WARNINGS
        _CONSOLE
        RTC_STATIC
        WIN32
    )
    # 使用 MD 运行时
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL" CACHE STRING "MSVC runtime")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-Wall -fPIC -pthread)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-Wall -fPIC -pthread)
endif()
