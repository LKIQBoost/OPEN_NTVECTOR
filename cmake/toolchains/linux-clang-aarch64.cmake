# Linux ARM64 (aarch64) Clang 交叉编译工具链
#
# 用法:
#   cmake -B build-linux-arm64 -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/linux-clang-aarch64.cmake
#
# 如需指定 sysroot:
#   cmake -B build-linux-arm64 \
#     -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/linux-clang-aarch64.cmake \
#     -DCMAKE_SYSROOT=/path/to/aarch64-linux-gnu-sysroot

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)

set(CMAKE_C_FLAGS_INIT "-target aarch64-linux-gnu")
set(CMAKE_CXX_FLAGS_INIT "-target aarch64-linux-gnu")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-fuse-ld=lld")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)
