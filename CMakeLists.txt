cmake_minimum_required(VERSION 3.20)
# SLikeNet 和 zlib 的 CMakeLists.txt 声明了很旧的 cmake_minimum_required(VERSION 2.6)
# CMake 4.x 不再兼容 < 3.5，设置此变量让它们能正常工作
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "" FORCE)
project(Program LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 优先使用项目内的 Find 模块（如 FindOpenSSL.cmake）
list(PREPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# 平台检测与编译选项
include(cmake/Platform.cmake)

# 依赖库查找 (OpenSSL 等必须在 SLikeNet 之前配置)
include(cmake/FindPrebuiltDeps.cmake)

# ============================================================
# 源码编译的第三方库
# ============================================================

# zlib 1.3.1
set(ZLIB_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SKIP_INSTALL_ALL ON CACHE BOOL "" FORCE)
add_subdirectory(application/include/zlib-1.3.1 EXCLUDE_FROM_ALL)

# SLikeNet (魔改版 RakNet)
# 只构建静态库，跳过示例和 DLL
set(SLIKENET_ENABLE_DLL OFF CACHE BOOL "" FORCE)
set(SLIKENET_ENABLE_SAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(application/include/SLikeNet-master EXCLUDE_FROM_ALL)

# ============================================================
# 主项目
# ============================================================
add_subdirectory(application)