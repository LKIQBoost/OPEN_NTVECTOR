cmake_minimum_required(VERSION 3.20)

# Python 3.12 embedding (local python.org install)
set(PY312_HOME "C:/Users/admin/AppData/Local/Programs/Python/Python312")

# 收集所有源文件
file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cc"
)
file(GLOB APP_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

# 排除 include/ 子目录中的源文件（那些是第三方库，单独构建）
list(FILTER APP_SOURCES EXCLUDE REGEX ".*/include/.*")

# 非 Windows 排除资源文件
if(NOT WIN32)
    list(FILTER APP_SOURCES EXCLUDE REGEX ".*\\.rc$")
endif()

# NtUniSdkBase/MPayDelegate/MPayWrapper 依赖 NtUniSDK DLL，仅在启用时编译
if(NOT ENABLE_NTUNISDK)
    list(FILTER APP_SOURCES EXCLUDE REGEX ".*/NtUniSdkBase\\.cpp$")
    list(FILTER APP_SOURCES EXCLUDE REGEX ".*/MPayDelegate\\.cpp$")
    list(FILTER APP_SOURCES EXCLUDE REGEX ".*/MPayWrapper\\.cpp$")
endif()

# rotormodule.c 通过 #include 在 PythonRuntime.h 中编译，排除独立编译
list(FILTER APP_SOURCES EXCLUDE REGEX ".*/rotormodule\\.c$")

# 创建可执行目标
add_executable(Program ${APP_SOURCES} ${APP_HEADERS})

# 编译定义
target_compile_definitions(Program PRIVATE
    OPENSSL_STATIC
    Py_ENABLE_SHARED
    RTC_STATIC
    JUICE_STATIC
    _RAKNET_LIB
    # Python 3: 使用 s#/y# 格式时必须有此宏，否则运行期抛
    # "PY_SSIZE_T_CLEAN macro must be defined for '#' formats"
    PY_SSIZE_T_CLEAN
)

if(WIN32)
    target_compile_definitions(Program PRIVATE
        _CONSOLE
        _CRT_SECURE_NO_WARNINGS
        _WINSOCK_DEPRECATED_NO_WARNINGS
    )

    # 可选: NtUniSdk 集成
    if(ENABLE_NTUNISDK)
        target_compile_definitions(Program PRIVATE Linker_NtUniSdk)
        target_sources(Program PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/NtUniSdkBase.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/MPayDelegate.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/MPayWrapper.cpp"
        )
    endif()
endif()

# Include 路径 (仅项目自身和无 CMake target 的第三方库)
# OpenSSL / jsoncpp / Python / libwebsockets / libdatachannel
# 的头文件由各自的 CMake target 通过 INTERFACE_INCLUDE_DIRECTORIES 自动提供
target_include_directories(Program PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/platform
    ${CMAKE_CURRENT_SOURCE_DIR}/include/SLikeNet-master/Source
    ${CMAKE_CURRENT_SOURCE_DIR}/include/SLikeNet-master/Source/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/zlib-1.3.1
    ${CMAKE_CURRENT_SOURCE_DIR}/include/pybind11
    ${PY312_HOME}/include
)

# 链接库 — 全部从源码编译
target_link_libraries(Program PRIVATE
    # 源码编译的库
    SLikeNetLibStatic
    zlibstatic

    # 源码编译的第三方库
    OpenSSL::SSL
    OpenSSL::Crypto
    jsoncpp_static
    ${PY312_HOME}/libs/python312.lib
    datachannel-static
    juice-static
    srtp2
    usrsctp
    websockets_static
    LibWebSocketWrapper
)

# 平台特定链接
if(WIN32)
    target_link_libraries(Program PRIVATE
        Ws2_32
        Crypt32
        advapi32
        user32
        gdi32
        bcrypt
    )

    if(ENABLE_NTUNISDK)
        target_link_libraries(Program PRIVATE NtUniSdkBase)
    endif()

    target_sources(Program PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Program.rc")
else()
    if(PLATFORM_ANDROID)
        target_link_libraries(Program PRIVATE dl m)
    else()
        target_link_libraries(Program PRIVATE pthread dl m)
    endif()
endif()

# MSVC 特定编译选项
if(MSVC)
    target_compile_options(Program PRIVATE
        /MP
        /W3
        /GS-
    )
    target_link_options(Program PRIVATE
        /OPT:REF
        /OPT:ICF
    )
else()
    target_compile_options(Program PRIVATE
        -fvisibility=hidden
        -fvisibility-inlines-hidden
    )
    target_link_options(Program PRIVATE
        -Wl,--exclude-libs,ALL
        -Wl,-s
    )
endif()
