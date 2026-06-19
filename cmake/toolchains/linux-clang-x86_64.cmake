# Linux x86_64 Clang 交叉编译工具链
#
# 用法:
#   cmake -B build-linux-x64 -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/linux-clang-x86_64.cmake
#
# 如需指定 sysroot:
#   cmake -B build-linux-x64 \
#     -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/linux-clang-x86_64.cmake \
#     -DCMAKE_SYSROOT=/path/to/x86_64-linux-gnu-sysroot

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)

set(CMAKE_C_FLAGS_INIT "-target x86_64-linux-gnu")
set(CMAKE_CXX_FLAGS_INIT "-target x86_64-linux-gnu")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-fuse-ld=lld")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
