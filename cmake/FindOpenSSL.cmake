# 自定义 FindOpenSSL.cmake
# 当 OpenSSL::SSL 和 OpenSSL::Crypto 已作为 IMPORTED target 存在时，直接标记为已找到
# 这样 SLikeNet 等子项目的 find_package(OpenSSL) 不会覆盖我们的配置

if(TARGET OpenSSL::SSL AND TARGET OpenSSL::Crypto)
    set(OPENSSL_FOUND TRUE)
    set(OpenSSL_FOUND TRUE)
    if(NOT OPENSSL_VERSION)
        set(OPENSSL_VERSION "3.5.1")
    endif()
    if(NOT OPENSSL_INCLUDE_DIR)
        get_target_property(OPENSSL_INCLUDE_DIR OpenSSL::Crypto INTERFACE_INCLUDE_DIRECTORIES)
    endif()
    return()
endif()

# 回退到系统默认的 FindOpenSSL
include(${CMAKE_ROOT}/Modules/FindOpenSSL.cmake)
