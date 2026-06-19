# Platform

Cross-platform C++ project built with CMake. Supports Windows (MSVC) and Linux (clang), targeting x86_64 and ARM64 architectures.

All dependencies are included in the repository and compiled from source — no external package manager required.

## Prerequisites

| Platform | Requirements |
|----------|-------------|
| **Windows** | Visual Studio 2015 ~ 2022, CMake 3.20+ |
| **Linux x86_64** | clang, cmake 3.20+, perl, make, pkg-config |
| **Linux ARM64** | Above + `gcc-aarch64-linux-gnu`, `g++-aarch64-linux-gnu` |

## Build

```bash
# Windows (from Developer Command Prompt)
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release

# Older VS versions
cmake -B build -G "Visual Studio 14 2015" -A x64
cmake --build build --config Release

# Linux x86_64
cmake -B build -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
cmake --build build -j$(nproc)

# Linux ARM64 (cross-compile)
cmake -B build -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/linux-clang-aarch64.cmake
cmake --build build -j$(nproc)
```

First build takes 5-10 minutes (OpenSSL + Python compile from source). Subsequent builds use cached results.

## Project Structure

```
Platform/
├── CMakeLists.txt              # Root CMake
├── cmake/
│   ├── Platform.cmake          # Compiler options
│   ├── FindPrebuiltDeps.cmake  # Dependency management
│   ├── BuildOpenSSL.cmake      # Auto-build OpenSSL at configure time
│   ├── BuildPython27.cmake     # Auto-build Python 2.7 (Linux)
│   └── toolchains/             # Cross-compilation toolchain files
├── application/                # Main application source
│   ├── include/
│   │   ├── SLikeNet-master/    # Networking library (source)
│   │   └── zlib-1.3.1/         # Compression (source)
│   └── platform/               # Platform-specific implementations
├── third_party/                # External dependencies (source)
│   ├── openssl/                # OpenSSL 3.5.0
│   ├── jsoncpp/                # JsonCpp 1.9.6
│   ├── libdatachannel/         # WebRTC data channels
│   └── libwebsockets/          # WebSocket protocol
└── Python-2.7.18/              # Embedded Python runtime (modified)
```

## Dependencies

All dependencies compile from source via CMake. No internet access needed at build time.

| Library | Version | Build Method |
|---------|---------|-------------|
| OpenSSL | 3.5.0 | Auto-built at cmake configure time |
| Python 2.7 | 2.7.18 (modified) | Auto-built on Linux; pre-built .lib on Windows |
| jsoncpp | 1.9.6 | `add_subdirectory` |
| libdatachannel | latest | `add_subdirectory` (includes libjuice, libsrtp, usrsctp) |
| libwebsockets | latest | `add_subdirectory` |
| SLikeNet | custom | `add_subdirectory` |
| zlib | 1.3.1 | `add_subdirectory` |

## License

See individual library directories for their respective licenses.
