# BuildPython27.cmake - 在 cmake configure 时自动构建 Python 2.7 (仅非 Windows)
#
# Windows 使用项目自带的 .lib 文件，不需要此脚本
#
# 输出变量:
#   PYTHON27_LIBRARY, PYTHON27_INCLUDE_DIR

set(_PYTHON_SRC "${CMAKE_SOURCE_DIR}/Python-2.7.18")
set(_PYTHON_BUILD "${CMAKE_BINARY_DIR}/_deps/python27-build")
set(_PYTHON_INSTALL "${CMAKE_BINARY_DIR}/_deps/python27")
set(_PYTHON_MARKER "${_PYTHON_INSTALL}/lib/libpython2.7.a")

if(EXISTS "${_PYTHON_MARKER}")
    message(STATUS "Python 2.7: using cached build at ${_PYTHON_INSTALL}")
else()
    if(NOT EXISTS "${_PYTHON_SRC}/configure")
        message(FATAL_ERROR
            "Python 2.7 source not found at ${_PYTHON_SRC}\n"
            "Please place your modified Python 2.7 source at that location.")
    endif()

    message(STATUS "")
    message(STATUS "========================================")
    message(STATUS "  Building Python 2.7 from source")
    message(STATUS "  (first time only, takes 1-3 minutes)")
    message(STATUS "========================================")
    message(STATUS "")

    file(MAKE_DIRECTORY "${_PYTHON_BUILD}")

    # CPU 核数
    cmake_host_system_information(RESULT _nproc QUERY NUMBER_OF_LOGICAL_CORES)
    if(NOT _nproc OR _nproc LESS 1)
        set(_nproc 4)
    endif()

    # 检测交叉编译
    set(_PY_CROSS_FLAGS "")
    set(_PY_CC "${CMAKE_C_COMPILER}")
    set(_PY_CXX "${CMAKE_CXX_COMPILER}")

    if(PLATFORM_ANDROID)
        # Termux 原生编译: Android Bionic libc 缺少大量 POSIX/GNU 函数
        file(WRITE "${_PYTHON_BUILD}/config.site"
"ac_cv_file__dev_ptmx=yes
ac_cv_file__dev_ptc=no
ac_cv_have_long_long_format=yes
ac_cv_buggy_getaddrinfo=no
ac_cv_lib_util_forkpty=no
ac_cv_func_forkpty=no
ac_cv_func_openpty=no
ac_cv_func_login_tty=no
ac_cv_func_getloadavg=no
ac_cv_func_setpwent=no
ac_cv_func_getpwent=no
ac_cv_func_endpwent=no
ac_cv_func_getspnam=no
ac_cv_func_getspent=no
ac_cv_func_setspent=no
ac_cv_func_endspent=no
ac_cv_header_shadow_h=no
ac_cv_func_clock_getres=no
ac_cv_lib_rt_clock_getres=no
")
        set(ENV{CONFIG_SITE} "${_PYTHON_BUILD}/config.site")
    endif()

    # 将项目依赖的 C 扩展模块静态编入 libpython2.7.a
    # Windows python27.lib 预编译时已内置这些模块; Linux/Android 从源码编译时
    # 默认会将它们编为 .so 动态扩展, 嵌入式使用时找不到, 必须通过 Setup.local 强制内置
    file(MAKE_DIRECTORY "${_PYTHON_BUILD}/Modules")
    file(WRITE "${_PYTHON_BUILD}/Modules/Setup.local"
"_struct _struct.c
binascii binascii.c
time timemodule.c
_collections _collectionsmodule.c
_io _io/bufferedio.c _io/bytesio.c _io/fileio.c _io/iobase.c _io/_iomodule.c _io/stringio.c _io/textio.c
itertools itertoolsmodule.c
operator operator.c
_functools _functoolsmodule.c
cStringIO cStringIO.c
cPickle cPickle.c
strop stropmodule.c
array arraymodule.c
_bisect _bisectmodule.c
_heapq _heapqmodule.c
_locale _localemodule.c
datetime datetimemodule.c
math mathmodule.c _math.c
_md5 md5module.c md5.c
_sha shamodule.c
_sha256 sha256module.c
_sha512 sha512module.c
zlib zlibmodule.c -lz
")

    # GCC 15+ 默认 C23，bool/true/false 是关键字，与 Python 2.7 冲突
    set(ENV{CC} "${_PY_CC}")
    set(ENV{CXX} "${_PY_CXX}")
    set(ENV{CFLAGS} "-fPIC -std=gnu11")

    # Configure
    execute_process(
        COMMAND "${_PYTHON_SRC}/configure"
            "--prefix=${_PYTHON_INSTALL}"
            --enable-shared=no
            --enable-unicode=ucs4
            ${_PY_CROSS_FLAGS}
        WORKING_DIRECTORY "${_PYTHON_BUILD}"
        RESULT_VARIABLE _result
    )
    if(_result)
        message(FATAL_ERROR "Python 2.7 configure failed (exit ${_result})")
    endif()

    # Build
    execute_process(
        COMMAND make -j${_nproc}
        WORKING_DIRECTORY "${_PYTHON_BUILD}"
        RESULT_VARIABLE _result
    )
    if(_result)
        message(FATAL_ERROR "Python 2.7 build failed (exit ${_result})")
    endif()

    # Install - 分步安装，跳过共享模块(可能因平台差异编译失败)
    # 1. 安装核心静态库和头文件
    execute_process(
        COMMAND make altbininstall inclinstall libainstall
        WORKING_DIRECTORY "${_PYTHON_BUILD}"
        RESULT_VARIABLE _result
    )
    if(_result)
        # 回退: 手动复制必要文件
        file(MAKE_DIRECTORY "${_PYTHON_INSTALL}/lib")
        file(MAKE_DIRECTORY "${_PYTHON_INSTALL}/include/python2.7")
        execute_process(COMMAND ${CMAKE_COMMAND} -E copy
            "${_PYTHON_BUILD}/libpython2.7.a" "${_PYTHON_INSTALL}/lib/libpython2.7.a")
        execute_process(COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${_PYTHON_SRC}/Include" "${_PYTHON_INSTALL}/include/python2.7")
        execute_process(COMMAND ${CMAKE_COMMAND} -E copy
            "${_PYTHON_BUILD}/pyconfig.h" "${_PYTHON_INSTALL}/include/python2.7/pyconfig.h")
    endif()

    message(STATUS "Python 2.7 build complete: ${_PYTHON_INSTALL}")
endif()

# 查找头文件目录
if(EXISTS "${_PYTHON_INSTALL}/include/python2.7/Python.h")
    set(PYTHON27_INCLUDE_DIR "${_PYTHON_INSTALL}/include/python2.7" CACHE PATH "" FORCE)
elseif(EXISTS "${_PYTHON_INSTALL}/include/Python.h")
    set(PYTHON27_INCLUDE_DIR "${_PYTHON_INSTALL}/include" CACHE PATH "" FORCE)
else()
    message(FATAL_ERROR "Python.h not found after building Python 2.7")
endif()

set(PYTHON27_LIBRARY "${_PYTHON_MARKER}" CACHE FILEPATH "" FORCE)

# 创建 IMPORTED target
if(NOT TARGET python27)
    add_library(python27 STATIC IMPORTED GLOBAL)
    set_target_properties(python27 PROPERTIES
        IMPORTED_LOCATION "${PYTHON27_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "$<BUILD_INTERFACE:${PYTHON27_INCLUDE_DIR}>"
    )
    if(PLATFORM_ANDROID)
        # Android/Bionic 没有 libutil; pthread/dl 内置于 libc 但 -l 标志无害
        target_link_libraries(python27 INTERFACE dl m)
    else()
        target_link_libraries(python27 INTERFACE dl pthread util m)
    endif()
endif()

# Linux 上 Python 扩展模块内置于 libpython2.7，不需要单独的 .lib
if(NOT TARGET python__socket)
    add_library(python__socket INTERFACE)
endif()
if(NOT TARGET python_select)
    add_library(python_select INTERFACE)
endif()
if(NOT TARGET python__ctypes)
    add_library(python__ctypes INTERFACE)
endif()
