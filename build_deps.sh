#!/bin/bash
# =============================================================
# build_deps.sh - 构建非 CMake 依赖 (OpenSSL, Python 2.7)
#
# 用法:
#   ./build_deps.sh [x86_64|aarch64] [Release|Debug]
# =============================================================
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ARCH="${1:-x86_64}"
CONFIG="${2:-Release}"

DEPS_DIR="$SCRIPT_DIR/../deps/$ARCH"
OPENSSL_SRC="$SCRIPT_DIR/third_party/openssl"
PYTHON_SRC="$SCRIPT_DIR/Python-2.7.18"
JOBS=$(nproc 2>/dev/null || echo 4)

echo "================================================================"
echo "Building dependencies for $ARCH $CONFIG"
echo "Output: $DEPS_DIR"
echo "Parallel jobs: $JOBS"
echo "================================================================"

mkdir -p "$DEPS_DIR"

# =============================================================
# 1. OpenSSL
# =============================================================
echo ""
echo "=== [1/2] Building OpenSSL ==="
echo ""

if [ -f "$DEPS_DIR/openssl/lib/libcrypto.a" ]; then
    echo "OpenSSL already built, skipping. Delete $DEPS_DIR/openssl to rebuild."
else
    cd "$OPENSSL_SRC"

    if [ "$ARCH" = "aarch64" ]; then
        OPENSSL_TARGET="linux-aarch64"
        export CC="${CC:-aarch64-linux-gnu-gcc}"
    else
        OPENSSL_TARGET="linux-x86_64"
        export CC="${CC:-clang}"
    fi

    ./Configure "$OPENSSL_TARGET" \
        --prefix="$DEPS_DIR/openssl" \
        no-shared no-tests no-dso \
        -fPIC

    make -j"$JOBS"
    make install_sw
    make clean

    echo "OpenSSL built successfully."
fi

# =============================================================
# 2. Python 2.7
# =============================================================
echo ""
echo "=== [2/2] Building Python 2.7 ==="
echo ""

if [ -f "$DEPS_DIR/python27/lib/libpython2.7.a" ]; then
    echo "Python 2.7 already built, skipping. Delete $DEPS_DIR/python27 to rebuild."
else
    if [ ! -d "$PYTHON_SRC" ]; then
        echo "ERROR: Python 2.7 source not found at $PYTHON_SRC"
        exit 1
    fi

    # 在临时目录中构建，避免污染源码
    PY_BUILD="$DEPS_DIR/python27-build"
    mkdir -p "$PY_BUILD"
    cd "$PY_BUILD"

    if [ "$ARCH" = "aarch64" ]; then
        CROSS_FLAGS="--host=aarch64-linux-gnu --build=x86_64-linux-gnu"
        export CC="aarch64-linux-gnu-gcc"
        export CXX="aarch64-linux-gnu-g++"
    else
        CROSS_FLAGS=""
        export CC="${CC:-clang}"
        export CXX="${CXX:-clang++}"
    fi

    "$PYTHON_SRC/configure" \
        --prefix="$DEPS_DIR/python27" \
        --enable-shared=no \
        --enable-unicode=ucs4 \
        $CROSS_FLAGS \
        CFLAGS="-fPIC"

    make -j"$JOBS"
    make install

    # 清理构建目录
    cd "$SCRIPT_DIR"
    rm -rf "$PY_BUILD"

    echo "Python 2.7 built successfully."
fi

echo ""
echo "================================================================"
echo "All dependencies built successfully!"
echo ""
echo "Now configure your project:"
if [ "$ARCH" = "aarch64" ]; then
    echo "  cmake -B build-linux-arm64 \\"
    echo "    -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/linux-clang-aarch64.cmake"
else
    echo "  cmake -B build-linux-x64 \\"
    echo "    -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++"
fi
echo "  cmake --build build-linux-x64 -j$JOBS"
echo "================================================================"
