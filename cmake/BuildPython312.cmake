# cmake/BuildPython312.cmake
#
# 在 configure 期从 third_party/Python-3.12.2 编出一个**静态** CPython 核心库
# python312core.lib,供 Program.exe 静态链接 —— 运行时不需要 python312.dll。
#
# 这是 Python 2.7 时代做法的 3.12 版本:当年用 PCbuild 的 pythoncore 工程出
# python27.lib(32 MB 静态库)静态链进 Program.exe。CPython 3.12 官方把
# pythoncore.vcxproj 改成了直接产 DLL,所以这里要自己生成一份静态化副本。
#
# 产物与缓存:
#   build/_deps/python-static/
#     proj/pythoncore_static.vcxproj   生成的补丁副本
#     out/amd64/python312core.lib      ← 缓存标记(marker)
#     obj/                             中间文件
#     freeze.log / core.log            构建日志
#   third_party/Python-3.12.2/Python/{frozen_modules/*.h,deepfreeze/deepfreeze.c}
#     由 _freeze_module 生成(源码 tarball 里没有,必须先跑),已 gitignore
#
# 输出:python312core IMPORTED target(含 include 路径与 Py_NO_ENABLE_SHARED)
#
# 关于 third_party/Python-3.12.2 的剪裁:为控制仓库体积,该树已从 python.org 的
# 源码 tarball 中删去与构建无关的部分 —— Doc/、Misc/、Mac/、Lib/test/、PCbuild/obj/
# (共省下约 53 MB,112 MB -> 59 MB)。**以下必须保留**:
#   PCbuild/{pythoncore,_freeze_module}.vcxproj、PCbuild/{python,pyproject}.props
#   Tools/build/{deepfreeze.py,umarshal.py,generate_global_objects.py}
#   Tools/freeze/flag.py(它是 frozen_only 这个冻结模块的源)
#   Include/、PC/、Modules/、Objects/、Parser/、Python/、Programs/、Grammar/、Lib/

if(NOT (WIN32 AND MSVC))
    # 非 Windows/MSVC 仍走原本的 find_package(Python3 ...) 路径
    return()
endif()

set(NTVECTOR_PY_SRC "${CMAKE_SOURCE_DIR}/third_party/Python-3.12.2")
set(_stage "${CMAKE_BINARY_DIR}/_deps/python-static")
set(_proj  "${_stage}/proj")
set(_out   "${_stage}/out")
set(_obj   "${_stage}/obj")
set(_lib   "${_out}/amd64/python312core.lib")
set(_deepfreeze "${NTVECTOR_PY_SRC}/Python/deepfreeze/deepfreeze.c")

# ============================================================
# 可配置项
# ============================================================

# 宿主 Python:只用来跑 Tools/build/deepfreeze.py。必须 >= 3.10。
set(NTVECTOR_PY_HOST "" CACHE FILEPATH
    "Host python.exe used by Tools/build/deepfreeze.py (>= 3.10). Auto-detected if left empty.")

# python.props 对 VS 15/16/17 之外的版本会静默回落到 v140(本机没装),
# 所以必须显式指定,不能留空。
set(NTVECTOR_PY_TOOLSET "${CMAKE_VS_PLATFORM_TOOLSET}" CACHE STRING
    "MSVC PlatformToolset used to build the static CPython core (e.g. v145).")
if(NOT NTVECTOR_PY_TOOLSET)
    set(NTVECTOR_PY_TOOLSET "v145" CACHE STRING
        "MSVC PlatformToolset used to build the static CPython core (e.g. v145)." FORCE)
endif()

set(NTVECTOR_PY_WINSDK "${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}" CACHE STRING
    "Windows SDK version used to build the static CPython core (e.g. 10.0.26100.0).")
if(NOT NTVECTOR_PY_WINSDK)
    set(NTVECTOR_PY_WINSDK "10.0.26100.0" CACHE STRING
        "Windows SDK version used to build the static CPython core (e.g. 10.0.26100.0)." FORCE)
endif()

option(NTVECTOR_PY_REBUILD "Force a rebuild of the static CPython core" OFF)

# ============================================================
# 前置校验
# ============================================================

foreach(_f PC/pyconfig.h PCbuild/pythoncore.vcxproj PCbuild/_freeze_module.vcxproj
           PCbuild/python.props PCbuild/pyproject.props)
    if(NOT EXISTS "${NTVECTOR_PY_SRC}/${_f}")
        message(FATAL_ERROR
            "Vendored CPython source incomplete: missing ${_f}\n"
            "Expected a Python-3.12.2 tree at ${NTVECTOR_PY_SRC}.")
    endif()
endforeach()

# ============================================================
# 工具链发现
# ============================================================

set(_msbuild "")
if(CMAKE_VS_MSBUILD_COMMAND AND EXISTS "${CMAKE_VS_MSBUILD_COMMAND}")
    set(_msbuild "${CMAKE_VS_MSBUILD_COMMAND}")
endif()
if(NOT _msbuild)
    # vswhere 是 VS 安装器自带的,位置固定
    set(_vswhere "$ENV{ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe")
    if(EXISTS "${_vswhere}")
        execute_process(
            COMMAND "${_vswhere}" -latest -products * -requires Microsoft.Component.MSBuild
                    -find MSBuild/**/Bin/MSBuild.exe
            OUTPUT_VARIABLE _vswhere_out
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET)
        if(_vswhere_out)
            string(REGEX REPLACE "[\r\n].*" "" _msbuild "${_vswhere_out}")
        endif()
    endif()
endif()
if(NOT _msbuild)
    find_program(_msbuild msbuild)
endif()
if(NOT _msbuild)
    message(FATAL_ERROR
        "MSBuild.exe not found. Tried CMAKE_VS_MSBUILD_COMMAND, vswhere.exe and PATH. "
        "A Visual Studio installation with the C++ toolset is required to build the "
        "static CPython core.")
endif()
cmake_path(NATIVE_PATH _msbuild _msbuild_native)

# 宿主 Python。
# 注意:这里必须先 find_program 到**临时变量**再回填 cache —— 若直接对
# NTVECTOR_PY_HOST 调用 find_program,上面那句 set(... CACHE ...) 已经把它定义
# 成了空字符串,而 CMake 认为"已定义(哪怕是空)"的变量无需再搜,会静默跳过。
if(NOT NTVECTOR_PY_HOST)
    find_program(_py_host_found NAMES python python3
        HINTS "$ENV{LOCALAPPDATA}/Programs/Python/Python312"
              "$ENV{LOCALAPPDATA}/Programs/Python/Python313"
              "$ENV{ProgramFiles}/Python312")
    if(NOT _py_host_found)
        # 最后试 py 启动器:它能直接报出默认解释器的路径
        find_program(_py_launcher NAMES py)
        if(_py_launcher)
            execute_process(COMMAND "${_py_launcher}" -3 -c "import sys; print(sys.executable)"
                OUTPUT_VARIABLE _py_host_found
                OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_QUIET)
        endif()
    endif()
    if(_py_host_found)
        set(NTVECTOR_PY_HOST "${_py_host_found}" CACHE FILEPATH
            "Host python.exe used by Tools/build/deepfreeze.py (>= 3.10)." FORCE)
    endif()
endif()
if(NOT NTVECTOR_PY_HOST OR NOT EXISTS "${NTVECTOR_PY_HOST}")
    message(FATAL_ERROR
        "Host Python not found (needed to run Tools/build/deepfreeze.py).\n"
        "Set -DNTVECTOR_PY_HOST=<path to python.exe> (must be >= 3.10).")
endif()
cmake_path(NATIVE_PATH NTVECTOR_PY_HOST _py_host_native)
execute_process(
    COMMAND "${NTVECTOR_PY_HOST}" -c "import sys; sys.exit(0 if sys.version_info[:2] >= (3,10) else 1)"
    RESULT_VARIABLE _py_ok ERROR_QUIET)
if(NOT _py_ok EQUAL 0)
    message(FATAL_ERROR
        "Host Python at ${NTVECTOR_PY_HOST} is older than 3.10; deepfreeze.py requires >= 3.10.")
endif()

# ============================================================
# 指纹:输入变了就忽略缓存标记重编
# ============================================================

set(_fingerprint_inputs
    "${NTVECTOR_PY_SRC}/PCbuild/pythoncore.vcxproj"
    "${NTVECTOR_PY_SRC}/PCbuild/_freeze_module.vcxproj"
    "${NTVECTOR_PY_SRC}/PC/pyconfig.h"
    "${CMAKE_CURRENT_LIST_FILE}"
)
set(_fingerprint_text "${NTVECTOR_PY_SRC}|${_msbuild_native}|${NTVECTOR_PY_TOOLSET}|${NTVECTOR_PY_WINSDK}|${CMAKE_GENERATOR}")
foreach(_f IN LISTS _fingerprint_inputs)
    file(SHA1 "${_f}" _h)
    string(APPEND _fingerprint_text "|${_h}")
endforeach()
string(SHA1 _fingerprint "${_fingerprint_text}")

set(_fp_file "${_stage}/fingerprint.txt")
set(_cached_fingerprint "")
if(EXISTS "${_fp_file}")
    file(READ "${_fp_file}" _cached_fingerprint)
    string(STRIP "${_cached_fingerprint}" _cached_fingerprint)
endif()

set(_lib_ok FALSE)
if(EXISTS "${_lib}" AND _cached_fingerprint STREQUAL _fingerprint AND NOT NTVECTOR_PY_REBUILD)
    set(_lib_ok TRUE)
endif()

# ============================================================
# 构建
# ============================================================

if(_lib_ok)
    message(STATUS "Static CPython core: using cached build at ${_lib}")
else()
    file(MAKE_DIRECTORY "${_proj}" "${_out}" "${_obj}")

    message(STATUS "")
    message(STATUS "========================================")
    message(STATUS "  Building static CPython 3.12 core")
    message(STATUS "  (first time only, takes a few minutes)")
    message(STATUS "========================================")
    message(STATUS "")

    # ---- 步骤 1:_freeze_module 生成 frozen_modules/*.h 与 deepfreeze.c ----
    # 这两个都不在源码 tarball 里(python.org 的 tarball 只带 README.txt),
    # 而 pythoncore 要编 deepfreeze.c,所以必须先跑这一步。
    # _freeze_module.vcxproj 自包含(无 ProjectReference,用 PC/config_minimal.c),
    # 且没有 Regen,所以可以直接原地用属性覆盖构建,不需要打补丁。
    if(NOT EXISTS "${_deepfreeze}")
        message(STATUS "Static CPython core: generating frozen modules + deepfreeze.c")
        execute_process(
            COMMAND "${_msbuild_native}"
                    "${NTVECTOR_PY_SRC}/PCbuild/_freeze_module.vcxproj"
                    -nologo -m -v:minimal
                    -p:Configuration=Release
                    -p:Platform=x64
                    -p:PlatformToolset=${NTVECTOR_PY_TOOLSET}
                    -p:WindowsTargetPlatformVersion=${NTVECTOR_PY_WINSDK}
                    -p:PythonForBuild=${_py_host_native}
                    -p:Py_OutDir=${_out}
                    -p:Py_IntDir=${_obj}/freeze
            RESULT_VARIABLE _r
            OUTPUT_FILE "${_stage}/freeze.log"
            ERROR_FILE "${_stage}/freeze.log"
            COMMAND_ECHO STDOUT)
        if(NOT _r EQUAL 0)
            message(FATAL_ERROR
                "Building _freeze_module failed (exit ${_r}). See ${_stage}/freeze.log")
        endif()
        if(NOT EXISTS "${_deepfreeze}")
            message(FATAL_ERROR
                "Frozen module generation produced no ${_deepfreeze}. See ${_stage}/freeze.log")
        endif()
    endif()

    # ---- 步骤 2:生成打过补丁的静态 pythoncore 工程 ----
    # 为什么必须打补丁(而不是只用 -p: 覆盖):
    #   a) ConfigurationType / TargetName 可以用全局属性覆盖,但 binascii.c 带
    #      USE_ZLIB_CRC32 需要 zlib.h,而该 include 只在 IncludeExternals=true 时
    #      才加入,命令行没有别的注入途径(INCLUDE 环境变量会被 MSBuild 的 CL
    #      task 覆盖,实测无效)。核心不能带 zlib 实现,否则与项目自己的
    #      zlibstatic 符号冲突,所以只能 IncludeExternals=false + 补 include 路径。
    #   b) _TriggerRegen 会让 Regen 每次构建都跑(内容 no-op,但依赖宿主 Python
    #      且拖慢构建),命令行无法禁用单个 MSBuild target。
    #   c) PC/python_nt.rc 会把 CPython 的 VERSIONINFO 合进 Program.exe 的资源。
    #   d) 工程被生成到 build 目录,所有相对 ..\ 路径与相对 Import 都要绝对化。
    file(READ "${NTVECTOR_PY_SRC}/PCbuild/pythoncore.vcxproj" _vcx)
    string(REPLACE "\\" "/" _py_src_fwd "${NTVECTOR_PY_SRC}")
    set(_zlib_inc "${CMAKE_SOURCE_DIR}/application/include/zlib-1.3.1;${CMAKE_BINARY_DIR}/application/include/zlib-1.3.1")

    # 必须用 function 而不是 macro:macro 的参数是文本替换,值里含反斜杠
    # (如 ..\Modules\x.c)时会被当作 CMake 转义序列重新解析并报
    # "Invalid character escape"。function 有真正的变量作用域,因此用 PARENT_SCOPE
    # 把改好的 XML 回传。含反斜杠的字面量一律用 bracket argument [==[...]==] 书写,
    # bracket argument 完全不处理转义。
    function(_py_patch vcx_in out_var old new label)
        string(FIND "${vcx_in}" "${old}" _pos)
        if(_pos EQUAL -1)
            message(FATAL_ERROR
                "Static CPython patch '${label}' failed: anchor not found in pythoncore.vcxproj.\n"
                "The vendored tree does not match Python 3.12.2 - re-extract it from the tarball.")
        endif()
        string(REPLACE "${old}" "${new}" _out "${vcx_in}")
        set(${out_var} "${_out}" PARENT_SCOPE)
    endfunction()

    set(_steps "")

    # 相对 Import -> 绝对(生成物不在 PCbuild/ 里了)
    _py_patch("${_vcx}" _vcx
        [==[<Import Project="python.props" />]==]
        "<Import Project=\"${_py_src_fwd}/PCbuild/python.props\" />"
        "import python.props")
    _py_patch("${_vcx}" _vcx
        [==[<Import Project="pyproject.props" />]==]
        "<Import Project=\"${_py_src_fwd}/PCbuild/pyproject.props\" />"
        "import pyproject.props")

    _py_patch("${_vcx}" _vcx
        "<ConfigurationType>DynamicLibrary</ConfigurationType>"
        "<ConfigurationType>StaticLibrary</ConfigurationType>"
        "ConfigurationType=StaticLibrary")
    _py_patch("${_vcx}" _vcx
        "<TargetName>$(PyDllName)</TargetName>"
        "<TargetName>python312core</TargetName>"
        "TargetName=python312core")

    # binascii.c 无条件带 USE_ZLIB_CRC32(需要 zlib.h),而 zlib 的 include 只在
    # IncludeExternals=true 时才加入。核心不能真带 zlib 实现(会和项目自己的
    # zlibstatic 符号打架),所以只补 include 路径,实现留给 zlibstatic。
    _py_patch("${_vcx}" _vcx
        [==[<AdditionalIncludeDirectories>$(PySourcePath)Modules\_hacl\include;$(PySourcePath)Modules\_hacl\internal;$(PySourcePath)Python;%(AdditionalIncludeDirectories)</AdditionalIncludeDirectories>]==]
        "<AdditionalIncludeDirectories>$(PySourcePath)Modules\\_hacl\\include;$(PySourcePath)Modules\\_hacl\\internal;$(PySourcePath)Python;${_zlib_inc};%(AdditionalIncludeDirectories)</AdditionalIncludeDirectories>"
        "zlib include path for binascii.c")

    _py_patch("${_vcx}" _vcx
        "    <Import Project=\"regen.targets\" />\n" "" "drop regen.targets import")
    _py_patch("${_vcx}" _vcx
        "  <Target Name=\"_TriggerRegen\" BeforeTargets=\"PrepareForBuild\" DependsOnTargets=\"Regen\" />\n"
        "" "drop _TriggerRegen")
    _py_patch("${_vcx}" _vcx
        [==[    <ResourceCompile Include="..\PC\python_nt.rc" />]==]
        "" "drop python_nt.rc (avoids injecting CPython VERSIONINFO)")

    string(REPLACE [==[..\]==] "${_py_src_fwd}/" _vcx "${_vcx}")
    string(APPEND _steps "  relative ..\\ -> absolute\n")

    file(WRITE "${_proj}/pythoncore_static.vcxproj" "${_vcx}")
    message(STATUS "Static CPython core: generated ${_proj}/pythoncore_static.vcxproj")
    message(STATUS "${_steps}")

    # ---- 步骤 3:构建静态核心 ----
    # 注意:核心**保留** Py_ENABLE_SHARED(工程自带),这既是 CPython 对 DLL 构建
    # 的设定,也是让 Py_EXPORTED_SYMBOL 非空、把 API 导出到 Program.exe 的前提。
    # Include/export.h: 没有 Py_ENABLE_SHARED 时 Py_EXPORTED_SYMBOL 是空的。
    # 应用侧则用 Py_NO_ENABLE_SHARED(见 application/CMakeLists.txt),两边不冲突。
    execute_process(
        COMMAND "${_msbuild_native}"
                "${_proj}/pythoncore_static.vcxproj"
                -nologo -m -v:minimal
                -p:Configuration=Release
                -p:Platform=x64
                -p:PlatformToolset=${NTVECTOR_PY_TOOLSET}
                -p:WindowsTargetPlatformVersion=${NTVECTOR_PY_WINSDK}
                -p:IncludeExternals=false
                -p:PythonForBuild=${_py_host_native}
                -p:Py_OutDir=${_out}
                -p:Py_IntDir=${_obj}/core
        RESULT_VARIABLE _r
        OUTPUT_FILE "${_stage}/core.log"
        ERROR_FILE "${_stage}/core.log"
        COMMAND_ECHO STDOUT)
    if(NOT _r EQUAL 0)
        message(FATAL_ERROR
            "Building the static CPython core failed (exit ${_r}). See ${_stage}/core.log")
    endif()
    if(NOT EXISTS "${_lib}")
        message(FATAL_ERROR
            "Static CPython core built but ${_lib} is missing. See ${_stage}/core.log")
    endif()

    file(WRITE "${_fp_file}" "${_fingerprint}")
    file(SIZE "${_lib}" _lib_size)
    math(EXPR _lib_mb "${_lib_size} / 1048576")
    message(STATUS "Static CPython core: built ${_lib} (${_lib_mb} MB)")
endif()

# ============================================================
# IMPORTED target
# ============================================================

if(NOT TARGET python312core)
    add_library(python312core STATIC IMPORTED GLOBAL)
    set_target_properties(python312core PROPERTIES
        IMPORTED_LOCATION "${_lib}"
        # Include/ 给 Python.h;PC/ 给 pyconfig.h(internal 头文件在 Include/internal,
        # 由 Include/ 里的相对 include 覆盖)
        INTERFACE_INCLUDE_DIRECTORIES "${NTVECTOR_PY_SRC}/Include;${NTVECTOR_PY_SRC}/PC"
        # 应用侧不能有 Py_ENABLE_SHARED,否则 PyAPI_FUNC 变成 dllimport,
        # 会去找一个并不存在的 python312.dll。
        INTERFACE_COMPILE_DEFINITIONS "Py_NO_ENABLE_SHARED"
    )
endif()
