# FindPrebuiltDeps.cmake → 统一依赖管理
#
# 所有依赖从源码编译，一条 cmake 命令搞定:
#   - OpenSSL / Python 2.7: configure 时自动构建 (缓存在 build/_deps/)
#   - jsoncpp / libdatachannel / libwebsockets: add_subdirectory 源码编译
#
# Windows 特殊处理:
#   - OpenSSL: 尝试自动构建，失败则回退到项目自带 .lib
#   - Python 2.7: 使用项目自带 .lib (VS2015+ Universal CRT 兼容)

set(APP_DIR "${CMAKE_SOURCE_DIR}/application")

# ============================================================
# 1. OpenSSL (自动构建)
# ============================================================
include(cmake/BuildOpenSSL.cmake)

# ============================================================
# 2. Python 2.7
# ============================================================
if(WIN32)
    # Windows: 使用项目自带的 .lib
    if(EXISTS "${APP_DIR}/python27.lib")
        message(STATUS "Python 2.7: using .lib from application/")

        add_library(python27 STATIC IMPORTED GLOBAL)
        set_target_properties(python27 PROPERTIES
            IMPORTED_LOCATION "${APP_DIR}/python27.lib"
            INTERFACE_INCLUDE_DIRECTORIES "${APP_DIR}/include/python2.7/Include"
        )

        # Python 扩展模块
        foreach(_pair "_socket;${APP_DIR}/_socket.lib" "select;${APP_DIR}/select.lib" "_ctypes;${APP_DIR}/_ctypes.lib")
            list(GET _pair 0 _name)
            list(GET _pair 1 _path)
            if(EXISTS "${_path}")
                add_library(python_${_name} STATIC IMPORTED GLOBAL)
                set_target_properties(python_${_name} PROPERTIES IMPORTED_LOCATION "${_path}")
            else()
                add_library(python_${_name} INTERFACE)
            endif()
        endforeach()
    else()
        message(FATAL_ERROR "Python 2.7 .lib not found at ${APP_DIR}/python27.lib")
    endif()
else()
    # Linux: 自动构建
    include(cmake/BuildPython27.cmake)
endif()

# ============================================================
# 3. jsoncpp (源码编译)
# ============================================================
set(JSONCPP_WITH_TESTS OFF CACHE BOOL "" FORCE)
set(JSONCPP_WITH_POST_BUILD_UNITTEST OFF CACHE BOOL "" FORCE)
set(JSONCPP_WITH_PKGCONFIG_SUPPORT OFF CACHE BOOL "" FORCE)
set(JSONCPP_WITH_CMAKE_PACKAGE OFF CACHE BOOL "" FORCE)
set(BUILD_OBJECT_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_STATIC_LIBS ON CACHE BOOL "" FORCE)

add_subdirectory("${CMAKE_SOURCE_DIR}/third_party/jsoncpp" "${CMAKE_BINARY_DIR}/third_party/jsoncpp" EXCLUDE_FROM_ALL)

if(TARGET jsoncpp_static)
    # ok
elseif(TARGET jsoncpp_lib)
    add_library(jsoncpp_static ALIAS jsoncpp_lib)
else()
    message(FATAL_ERROR "jsoncpp target not found after add_subdirectory")
endif()

# ============================================================
# 4. libwebsockets (源码编译)
# ============================================================
set(LWS_WITH_STATIC ON CACHE BOOL "" FORCE)
set(LWS_WITH_SHARED OFF CACHE BOOL "" FORCE)
set(LWS_WITHOUT_TESTAPPS ON CACHE BOOL "" FORCE)
set(LWS_WITHOUT_TEST_SERVER ON CACHE BOOL "" FORCE)
set(LWS_WITHOUT_TEST_CLIENT ON CACHE BOOL "" FORCE)
set(LWS_WITHOUT_TEST_PING ON CACHE BOOL "" FORCE)
set(LWS_WITH_MINIMAL_EXAMPLES OFF CACHE BOOL "" FORCE)
set(LWS_WITH_SSL ON CACHE BOOL "" FORCE)
set(LWS_WITH_GNUTLS OFF CACHE BOOL "" FORCE)
set(LWS_WITH_MBEDTLS OFF CACHE BOOL "" FORCE)
set(LWS_SSL_CLIENT_USE_OS_CA_CERTS OFF CACHE BOOL "" FORCE)
set(LWS_WITH_BUNDLED_ZLIB OFF CACHE BOOL "" FORCE)
set(DISABLE_WERROR ON CACHE BOOL "" FORCE)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW CACHE STRING "" FORCE)

add_subdirectory("${CMAKE_SOURCE_DIR}/third_party/libwebsockets" "${CMAKE_BINARY_DIR}/third_party/libwebsockets" EXCLUDE_FROM_ALL)

if(NOT TARGET websockets_static)
    if(TARGET websockets)
        add_library(websockets_static ALIAS websockets)
    else()
        message(FATAL_ERROR "libwebsockets target not found")
    endif()
endif()

# LibWebSocketWrapper — 全平台从源码编译，隔离 libwebsockets 头文件
# WebSocketClient.h 只暴露纯虚接口，lws 符号不泄露到主项目
add_library(LibWebSocketWrapper STATIC
    "${APP_DIR}/platform/LinuxWebSocketClient.cpp"
)
target_link_libraries(LibWebSocketWrapper PUBLIC websockets_static)
target_include_directories(LibWebSocketWrapper PRIVATE "${APP_DIR}")

# ============================================================
# 5. libdatachannel (源码编译, 含 libjuice/srtp2/usrsctp)
# ============================================================
set(NO_MEDIA ON CACHE BOOL "" FORCE)
set(NO_WEBSOCKET ON CACHE BOOL "" FORCE)
set(NO_EXAMPLES ON CACHE BOOL "" FORCE)
set(NO_TESTS ON CACHE BOOL "" FORCE)

add_subdirectory("${CMAKE_SOURCE_DIR}/third_party/libdatachannel" "${CMAKE_BINARY_DIR}/third_party/libdatachannel" EXCLUDE_FROM_ALL)

if(NOT TARGET juice-static)
    add_library(juice-static INTERFACE)
endif()
if(NOT TARGET srtp2)
    add_library(srtp2 INTERFACE)
endif()
if(NOT TARGET usrsctp)
    add_library(usrsctp INTERFACE)
endif()

# ============================================================
# 6. NtUniSdkBase (Windows-only, optional)
# ============================================================
option(ENABLE_NTUNISDK "Enable NtUniSdk integration (Windows only)" OFF)
if(ENABLE_NTUNISDK AND WIN32)
    if(EXISTS "${APP_DIR}/NtUniSdkBase.lib")
        add_library(NtUniSdkBase STATIC IMPORTED GLOBAL)
        set_target_properties(NtUniSdkBase PROPERTIES
            IMPORTED_LOCATION "${APP_DIR}/NtUniSdkBase.lib"
        )
    endif()
endif()
