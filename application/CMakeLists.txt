cmake_minimum_required(VERSION 3.20)

# Python 运行时。Windows/MSVC 下从 third_party/Python-3.12.2 编出静态核心
# (python312core,见 cmake/BuildPython312.cmake),运行时不需要 python312.dll。
# 其他平台没有静态核心,回落到本机开发安装的动态库。
if(TARGET python312core)
    set(NTVECTOR_PY_LINK python312core)
    set(NTVECTOR_PY_INCLUDES "")
else()
    find_package(Python3 3.12 COMPONENTS Development REQUIRED)
    set(NTVECTOR_PY_LINK Python3::Python)
    set(NTVECTOR_PY_INCLUDES ${Python3_INCLUDE_DIRS})
endif()

# 收集所有源文件
file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cc"
)
file(GLOB_RECURSE APP_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

# Legacy flat header paths are compatibility shims; compiled implementations live
# only in the structured directories below.
set(_STRUCTURED_DIRS Core Crypto Network Packets Python PythonAPI NBT platform)
set(_STRUCTURED_NAMES)
foreach(_dir IN LISTS _STRUCTURED_DIRS)
    file(GLOB _structured_files "${CMAKE_CURRENT_SOURCE_DIR}/${_dir}/*.cpp"
         "${CMAKE_CURRENT_SOURCE_DIR}/${_dir}/*.c"
         "${CMAKE_CURRENT_SOURCE_DIR}/${_dir}/*.cc")
    foreach(_structured_file IN LISTS _structured_files)
        get_filename_component(_structured_name "${_structured_file}" NAME)
        list(APPEND _STRUCTURED_NAMES "${_structured_name}")
    endforeach()
endforeach()
list(REMOVE_DUPLICATES _STRUCTURED_NAMES)
foreach(_structured_name IN LISTS _STRUCTURED_NAMES)
    list(FILTER APP_SOURCES EXCLUDE REGEX "^${CMAKE_CURRENT_SOURCE_DIR}/${_structured_name}$")
endforeach()

# 排除 include/ 子目录中的源文件（那些是第三方库，单独构建）
list(FILTER APP_SOURCES EXCLUDE REGEX ".*/include/.*")
list(FILTER APP_HEADERS EXCLUDE REGEX ".*/include/.*")

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
    RTC_STATIC
    JUICE_STATIC
    _RAKNET_LIB
    # Python 3: 使用 s#/y# 格式时必须有此宏，否则运行期抛
    # "PY_SSIZE_T_CLEAN macro must be defined for '#' formats"
    PY_SSIZE_T_CLEAN
)

# 链接方式必须和 Python 运行时一致:
#   静态核心 -> Py_NO_ENABLE_SHARED(PyAPI_FUNC 不是 dllimport)。这条由
#               python312core 的 INTERFACE_COMPILE_DEFINITIONS 提供,从这里
#               到所有 TU 都生效,包括只经 pybind11 间接包含 Python.h 的那些。
#   动态库   -> Py_ENABLE_SHARED。
# 两边搞反的后果:静态侧写成 Py_ENABLE_SHARED 会去找不存在的 python312.dll;
# 动态侧写成 Py_NO_ENABLE_SHARED 会拿不到 dllimport 声明而链接失败。
if(NOT TARGET python312core)
    target_compile_definitions(Program PRIVATE Py_ENABLE_SHARED)
else()
    # 静态核心下 PyMODINIT_FUNC 展开成 extern "C" PyObject*(没有 dllexport),
    # 而 pybind11 的 PYBIND11_MODULE 默认给 PyInit_xxx 挂 __declspec(dllexport),
    # 两者属性不一致时 MSVC 报 C2375"重定义;不同的链接"。工程自己的头文件
    # (engine.h / PythonApi.h)用 PyMODINIT_FUNC 声明,pybind11 那 5 个模块
    # (nbt/packets/game_state/client_instance/easy_utils)就撞在这上面。
    # 动态链接时 PyMODINIT_FUNC 自带 dllexport 掩盖了这个问题。
    # pybind11 用 #if !defined(PYBIND11_EXPORT) 保护该宏,置空即可对齐;何况
    # 本工程的模块是靠 sys.modules / inittab 注册的,本来也不需要导出。
    target_compile_definitions(Program PRIVATE PYBIND11_EXPORT=)
endif()

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
            "${CMAKE_CURRENT_SOURCE_DIR}/Core/NtUniSdkBase.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/Core/MPayDelegate.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/Core/MPayWrapper.cpp"
        )
    endif()
endif()

# Include 路径 (仅项目自身和无 CMake target 的第三方库)
# OpenSSL / jsoncpp / Python / libwebsockets / libdatachannel
# 的头文件由各自的 CMake target 通过 INTERFACE_INCLUDE_DIRECTORIES 自动提供
target_include_directories(Program PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Core
    ${CMAKE_CURRENT_SOURCE_DIR}/Crypto
    ${CMAKE_CURRENT_SOURCE_DIR}/Network
    ${CMAKE_CURRENT_SOURCE_DIR}/Packets
    ${CMAKE_CURRENT_SOURCE_DIR}/Python
    ${CMAKE_CURRENT_SOURCE_DIR}/PythonAPI
    ${CMAKE_CURRENT_SOURCE_DIR}/NBT
    ${CMAKE_CURRENT_SOURCE_DIR}/platform
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include/SLikeNet-master/Source
    ${CMAKE_CURRENT_SOURCE_DIR}/include/SLikeNet-master/Source/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/zlib-1.3.1
    ${CMAKE_CURRENT_SOURCE_DIR}/include/pybind11
    ${NTVECTOR_PY_INCLUDES}
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
    ${NTVECTOR_PY_LINK}
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
        # 静态 CPython 核心要求的额外系统库,对应 pythoncore.vcxproj 里的
        # <AdditionalDependencies>version.lib;ws2_32.lib;pathcch.lib;bcrypt.lib</AdditionalDependencies>
        #   version  -> sysmodule.obj 的 GetFileVersionInfoW/VerQueryValueW
        #   pathcch  -> posixmodule.obj / fileutils.obj 的 PathCchSkipRoot 等
        version
        pathcch
    )

    if(ENABLE_NTUNISDK)
        target_link_libraries(Program PRIVATE NtUniSdkBase)
    endif()

    if(MSVC)
        target_sources(Program PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Program.rc")
    endif()
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
        # 不要 ICF。静态链接后 ~1700 个 CPython API 符号进了同一个映像,其中大量
        # clinic 生成的函数体字节完全相同,ICF 会把它们折叠到同一地址,使本应不同
        # 的函数指针相等。CPython 自己的构建就是 /OPT:REF,NOICF(pyproject.props:89)。
        /OPT:NOICF
        # 让 exe 产出导入库。核心的 API 经 dllexport 已经导出到 Program.exe,
        # Program.lib 供"用户自编 C 扩展 .pyd 从已加载的 exe 导入符号"使用。
        "/IMPLIB:${CMAKE_BINARY_DIR}/application/$<CONFIG>/Program.lib"
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

# 静态嵌入下没有 .pyd 可加载,CPython 3.12 拆出去的那些 stdlib C 扩展
# (_socket / select / _queue / zlib / ...)必须编进 Program.exe 并注册到
# builtin import table。白名单见 cmake/PythonBuiltinModules.cmake。
include(${CMAKE_SOURCE_DIR}/cmake/PythonBuiltinModules.cmake)
