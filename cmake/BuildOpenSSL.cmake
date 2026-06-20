# BuildOpenSSL.cmake - 在 cmake configure 时自动构建 OpenSSL
#
# Windows: 需要 Perl + nmake (从 VS Developer Command Prompt 运行)
#          如果环境不满足，回退到项目自带的 .lib
# Linux:   需要 Perl + make (自动构建)
#
# 输出: OpenSSL::SSL, OpenSSL::Crypto IMPORTED targets

set(_OPENSSL_SRC "${CMAKE_SOURCE_DIR}/third_party/openssl")

if(NOT EXISTS "${_OPENSSL_SRC}/Configure")
    message(FATAL_ERROR "OpenSSL source not found at ${_OPENSSL_SRC}")
endif()

set(_OPENSSL_BUILD "${CMAKE_BINARY_DIR}/_deps/openssl-build")
set(_OPENSSL_INSTALL "${CMAKE_BINARY_DIR}/_deps/openssl")

if(WIN32)
    set(_OPENSSL_MARKER "${_OPENSSL_INSTALL}/lib/libssl.lib")
else()
    # OpenSSL 3.x 可能安装到 lib/ 或 lib64/，检测两个位置
    if(EXISTS "${_OPENSSL_INSTALL}/lib64/libssl.a")
        set(_OPENSSL_LIBDIR "${_OPENSSL_INSTALL}/lib64")
    else()
        set(_OPENSSL_LIBDIR "${_OPENSSL_INSTALL}/lib")
    endif()
    set(_OPENSSL_MARKER "${_OPENSSL_LIBDIR}/libssl.a")
endif()

set(_OPENSSL_BUILD_SUCCESS FALSE)

if(EXISTS "${_OPENSSL_MARKER}")
    message(STATUS "OpenSSL: using cached build at ${_OPENSSL_INSTALL}")
    set(_OPENSSL_BUILD_SUCCESS TRUE)
else()
    find_program(PERL_EXECUTABLE perl)
    if(NOT PERL_EXECUTABLE)
        if(WIN32)
            message(STATUS "OpenSSL: Perl not found, will try legacy .lib fallback")
        else()
            message(FATAL_ERROR "Perl not found. Install perl to build OpenSSL.")
        endif()
    else()
        message(STATUS "")
        message(STATUS "========================================")
        message(STATUS "  Building OpenSSL from source")
        message(STATUS "  (first time only, takes 1-3 minutes)")
        message(STATUS "========================================")
        message(STATUS "")

        file(MAKE_DIRECTORY "${_OPENSSL_BUILD}")

        # 检测目标
        if(WIN32)
            if(CMAKE_SIZEOF_VOID_P EQUAL 8)
                set(_OPENSSL_TARGET "VC-WIN64A")
            else()
                set(_OPENSSL_TARGET "VC-WIN32")
            endif()
            set(_OPENSSL_EXTRA_FLAGS "no-asm")
            set(_OPENSSL_PIC_FLAG "")
        elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64|ARM64")
            set(_OPENSSL_TARGET "linux-aarch64")
            set(_OPENSSL_EXTRA_FLAGS "no-asm")
            set(_OPENSSL_PIC_FLAG "-fPIC")
        else()
            set(_OPENSSL_TARGET "linux-x86_64")
            set(_OPENSSL_EXTRA_FLAGS "")
            set(_OPENSSL_PIC_FLAG "-fPIC")
        endif()

        # Configure
        set(_OPENSSL_CONFIGURE_ARGS
            "${_OPENSSL_SRC}/Configure"
            ${_OPENSSL_TARGET}
            "--prefix=${_OPENSSL_INSTALL}"
            no-shared no-tests no-dso
            ${_OPENSSL_EXTRA_FLAGS}
            ${_OPENSSL_PIC_FLAG}
        )
        execute_process(
            COMMAND ${PERL_EXECUTABLE} ${_OPENSSL_CONFIGURE_ARGS}
            WORKING_DIRECTORY "${_OPENSSL_BUILD}"
            RESULT_VARIABLE _result
            COMMAND_ECHO STDOUT
        )
        if(_result)
            if(WIN32)
                message(WARNING "OpenSSL Configure failed, will try legacy .lib fallback")
            else()
                message(FATAL_ERROR "OpenSSL Configure failed (exit ${_result})")
            endif()
        else()
            # 检测 CPU 数
            cmake_host_system_information(RESULT _nproc QUERY NUMBER_OF_LOGICAL_CORES)
            if(NOT _nproc OR _nproc LESS 1)
                set(_nproc 4)
            endif()

            # Build
            if(WIN32)
                execute_process(
                    COMMAND nmake
                    WORKING_DIRECTORY "${_OPENSSL_BUILD}"
                    RESULT_VARIABLE _result
                )
            else()
                execute_process(
                    COMMAND make -j${_nproc}
                    WORKING_DIRECTORY "${_OPENSSL_BUILD}"
                    RESULT_VARIABLE _result
                )
            endif()

            if(_result)
                if(WIN32)
                    message(WARNING "OpenSSL build failed, will try legacy .lib fallback")
                else()
                    message(FATAL_ERROR "OpenSSL build failed (exit ${_result})")
                endif()
            else()
                # Install
                if(WIN32)
                    execute_process(
                        COMMAND nmake install_sw
                        WORKING_DIRECTORY "${_OPENSSL_BUILD}"
                        RESULT_VARIABLE _result
                    )
                else()
                    execute_process(
                        COMMAND make install_sw
                        WORKING_DIRECTORY "${_OPENSSL_BUILD}"
                        RESULT_VARIABLE _result
                    )
                endif()

                if(_result)
                    message(WARNING "OpenSSL install failed")
                else()
                    set(_OPENSSL_BUILD_SUCCESS TRUE)
                    # 重新检测 lib 目录 (可能是 lib64)
                    if(NOT WIN32)
                        if(EXISTS "${_OPENSSL_INSTALL}/lib64/libssl.a")
                            set(_OPENSSL_LIBDIR "${_OPENSSL_INSTALL}/lib64")
                        else()
                            set(_OPENSSL_LIBDIR "${_OPENSSL_INSTALL}/lib")
                        endif()
                        set(_OPENSSL_MARKER "${_OPENSSL_LIBDIR}/libssl.a")
                    endif()
                    message(STATUS "OpenSSL build complete: ${_OPENSSL_INSTALL}")
                endif()
            endif()
        endif()
    endif()
endif()

# 设置 IMPORTED targets
set(APP_DIR "${CMAKE_SOURCE_DIR}/application")

if(_OPENSSL_BUILD_SUCCESS)
    # 使用自编译的 OpenSSL
    set(OPENSSL_ROOT_DIR "${_OPENSSL_INSTALL}" CACHE PATH "" FORCE)
    set(OPENSSL_INCLUDE_DIR "${_OPENSSL_INSTALL}/include" CACHE PATH "" FORCE)

    if(WIN32)
        set(OPENSSL_SSL_LIBRARY "${_OPENSSL_INSTALL}/lib/libssl.lib" CACHE FILEPATH "" FORCE)
        set(OPENSSL_CRYPTO_LIBRARY "${_OPENSSL_INSTALL}/lib/libcrypto.lib" CACHE FILEPATH "" FORCE)
    else()
        # OpenSSL 3.x 可能安装到 lib/ 或 lib64/
        if(EXISTS "${_OPENSSL_INSTALL}/lib64/libssl.a")
            set(_ssl_libdir "${_OPENSSL_INSTALL}/lib64")
        else()
            set(_ssl_libdir "${_OPENSSL_INSTALL}/lib")
        endif()
        set(OPENSSL_SSL_LIBRARY "${_ssl_libdir}/libssl.a" CACHE FILEPATH "" FORCE)
        set(OPENSSL_CRYPTO_LIBRARY "${_ssl_libdir}/libcrypto.a" CACHE FILEPATH "" FORCE)
    endif()

    set(OPENSSL_FOUND TRUE CACHE BOOL "" FORCE)
    set(OPENSSL_VERSION "3.5.0" CACHE STRING "" FORCE)
    set(OPENSSL_LIBRARIES "${OPENSSL_SSL_LIBRARY};${OPENSSL_CRYPTO_LIBRARY}" CACHE STRING "" FORCE)

    if(NOT TARGET OpenSSL::Crypto)
        add_library(OpenSSL::Crypto STATIC IMPORTED GLOBAL)
        set_target_properties(OpenSSL::Crypto PROPERTIES
            IMPORTED_LOCATION "${OPENSSL_CRYPTO_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "$<BUILD_INTERFACE:${OPENSSL_INCLUDE_DIR}>"
        )
        if(NOT WIN32)
            if(PLATFORM_ANDROID)
                target_link_libraries(OpenSSL::Crypto INTERFACE dl)
            else()
                target_link_libraries(OpenSSL::Crypto INTERFACE dl pthread)
            endif()
        endif()
    endif()
    if(NOT TARGET OpenSSL::SSL)
        add_library(OpenSSL::SSL STATIC IMPORTED GLOBAL)
        set_target_properties(OpenSSL::SSL PROPERTIES
            IMPORTED_LOCATION "${OPENSSL_SSL_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "$<BUILD_INTERFACE:${OPENSSL_INCLUDE_DIR}>"
        )
        target_link_libraries(OpenSSL::SSL INTERFACE OpenSSL::Crypto)
    endif()

elseif(WIN32 AND EXISTS "${APP_DIR}/libssl_static.lib")
    # Windows 回退: 使用项目自带的预编译 .lib
    message(STATUS "OpenSSL: using legacy .lib files from application/")

    set(OPENSSL_INCLUDE_DIR "${APP_DIR}/include/openssl_3.5.1" CACHE PATH "" FORCE)
    set(OPENSSL_SSL_LIBRARY "${APP_DIR}/libssl_static.lib" CACHE FILEPATH "" FORCE)
    set(OPENSSL_CRYPTO_LIBRARY "${APP_DIR}/libcrypto_static.lib" CACHE FILEPATH "" FORCE)
    set(OPENSSL_FOUND TRUE CACHE BOOL "" FORCE)
    set(OPENSSL_VERSION "3.5.1" CACHE STRING "" FORCE)
    set(OPENSSL_LIBRARIES "${OPENSSL_SSL_LIBRARY};${OPENSSL_CRYPTO_LIBRARY}" CACHE STRING "" FORCE)
    set(OPENSSL_ROOT_DIR "${OPENSSL_INCLUDE_DIR}" CACHE PATH "" FORCE)

    if(NOT TARGET OpenSSL::Crypto)
        add_library(OpenSSL::Crypto STATIC IMPORTED GLOBAL)
        set_target_properties(OpenSSL::Crypto PROPERTIES
            IMPORTED_LOCATION "${OPENSSL_CRYPTO_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "$<BUILD_INTERFACE:${OPENSSL_INCLUDE_DIR}>"
        )
    endif()
    if(NOT TARGET OpenSSL::SSL)
        add_library(OpenSSL::SSL STATIC IMPORTED GLOBAL)
        set_target_properties(OpenSSL::SSL PROPERTIES
            IMPORTED_LOCATION "${OPENSSL_SSL_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "$<BUILD_INTERFACE:${OPENSSL_INCLUDE_DIR}>"
        )
        target_link_libraries(OpenSSL::SSL INTERFACE OpenSSL::Crypto)
    endif()

else()
    message(FATAL_ERROR
        "OpenSSL not available.\n"
        "  Windows: install Perl, run from VS Developer Command Prompt\n"
        "  Linux: install perl and build tools")
endif()
