# NTVECTOR_Platform

跨平台 **Minecraft 基岩版客户端平台**，基于 C++17 与 CMake 构建。

支持 **Windows (MSVC)**、**Linux x86_64 (GCC / Clang)** 与 **Android ARM64 (Termux)** 三种平台。
所有第三方依赖均随仓库提供并从源码编译，构建时**无需外部包管理器、无需联网**。

## 主要特性

- **Minecraft 基岩版协议客户端**：登录 / 鉴权 / 数据包编解码
- **多种网络后端**：
  - SLikeNet（RakNet 魔改分支）— 联机通信
  - libwebsockets — WebSocket 协议
  - libdatachannel — WebRTC DataChannel（含 libjuice / libsrtp / usrsctp）
  - NetherNet 连接模式（`--start_nethernet`）
- **内嵌 Python 2.7 运行时**（静态链接），提供 `engine` / `client_instance` / `websocket` / `web_rtc` / `raknet` 等大量 Python 绑定模块
- **加密与鉴权**：AES-GCM / AES-CTR、ChaCha20、ECC / ECDSA 密钥交换、Base64
- **MCP 文件系统模式**：可由 `client_cfg.json` 中的 `no_launch_mcp` 字段控制
- **皮肤系统**：皮肤数据解析、皮肤 / 模型文件加载与转换

## 环境要求

| 平台 | 要求 |
|------|------|
| **Windows** | Visual Studio 2022、CMake 3.20+ |
| **Linux x86_64** | clang / gcc、CMake 3.20+、perl、make、pkg-config |
| **Android ARM64** | Termux 原生编译（clang），Bionic libc |

> 从 Windows 交叉编译 Linux x86_64 二进制时，使用 `cmake/toolchains/linux-clang-x86_64.cmake`（clang + lld，需提供目标 sysroot）。

## 构建

```bash
# Windows（在"开发者命令提示符"中执行）
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release

# Linux x86_64（原生编译）
cmake -B build -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
cmake --build build -j$(nproc)

# Linux x86_64（从 Windows 交叉编译，需指定 sysroot）
cmake -B build-linux-x64 \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/linux-clang-x86_64.cmake \
  -DCMAKE_SYSROOT=/path/to/x86_64-linux-gnu-sysroot
cmake --build build-linux-x64

# Android / Termux（原生编译）
pkg install clang cmake make perl pkg-config
cmake -B build
cmake --build build -j$(nproc)
```

首次构建约需 **5-10 分钟**（OpenSSL 与 Python 2.7 需要从源码编译）；后续构建会复用缓存，速度明显加快。

## 项目结构

```
NTVECTOR_Platform/
├── CMakeLists.txt              # 根 CMake 配置
├── cmake/
│   ├── Platform.cmake          # 平台检测与编译器选项
│   ├── FindPrebuiltDeps.cmake  # 依赖统一管理
│   ├── BuildOpenSSL.cmake      # cmake 配置期自动编译 OpenSSL
│   ├── BuildPython27.cmake     # 自动编译 Python 2.7（Linux / Android）
│   └── toolchains/             # 交叉编译工具链文件
├── application/                # 主程序源码（目标名：Program）
│   ├── *.cpp / *.h             # 协议、网络、登录、加密、Python 绑定等
│   ├── include/                # SLikeNet-master、zlib、pybind11、Python2.7 头文件
│   └── platform/               # 平台相关实现（如 cxa_stub.cpp）
├── third_party/                # 第三方依赖源码
│   ├── openssl/                # OpenSSL（自动构建）
│   ├── jsoncpp/                # JSON 解析
│   ├── libdatachannel/         # WebRTC DataChannel
│   └── libwebsockets/          # WebSocket
└── Python-2.7.18/              # 内嵌 Python 2.7 运行时（定制）
```

## 依赖

所有依赖均从源码编译，构建过程不需要联网下载。

| 库 | 版本 | 构建方式 |
|----|------|----------|
| OpenSSL | 3.5.0 | cmake 配置期自动构建；Windows 失败时回退到项目自带 `.lib` |
| Python 2.7 | 2.7.18（定制） | Linux / Android 从源码构建；Windows 使用预编译 `.lib` |
| jsoncpp | 1.9.6 | `add_subdirectory` |
| libdatachannel | 最新 | `add_subdirectory`（含 libjuice、libsrtp、usrsctp） |
| libwebsockets | 4.5 | `add_subdirectory` |
| SLikeNet | 定制（RakNet 魔改） | `add_subdirectory` |
| zlib | 1.3.1 | `add_subdirectory` |

## 配置文件

| 文件 | 作用 |
|------|------|
| `mc.cfg` | 启动配置（JSON）：`room_info`（服务器 IP / 端口）、`player_info`（账号 / 昵称 / token）、`misc`（鉴权服务器地址、引擎版本、网易 SID 等）、`skin_info`（皮肤 / 模型路径） |
| `client_cfg.json` | 客户端配置（JSON）：`no_launch_mcp` 是否启用 MCP 文件系统模式 |
| `skin_data.json` | 皮肤数据 |

> 旧版本中的 `options.txt` 已废弃并入 `client_cfg.json`。

## 命令行参数

| 分类 | 参数 | 作用 |
|------|------|------|
| 启动 | `--config <file>` | 指定启动配置文件（默认 `mc.cfg`） |
| | `--start_nethernet` | NetherNet 直连模式启动 |
| | `--do_main <event>` | 触发指定主事件 |
| | `--start_from_launcher=1` | 由启动器拉起 |
| 账号 / 鉴权 | `--UserID <id>` | 用户 ID |
| | `--DisplayName <name>` | 显示名称 |
| | `--MD5Token <token>` | 鉴权 token |
| | `--xuid <xuid>` | Xbox / 网易 XUID |
| | `--operator_name <name>` | 操作员名称 |
| | `--operator_uid <uid>` | 操作员 UID |
| | `--AuthServerUrl <url>` | 鉴权服务器地址 |
| | `--auto_auth_input` | 自动鉴权输入 |
| 服务器 | `--ServerIP <ip>` | 服务器 IP |
| | `--ServerPort <port>` | 服务器端口 |
| | `--NeteaseServerID <sid>` | 网易服务器 SID |
| | `--HostNethernetId <id>` | 主机 NetherNet 会话 ID |
| | `--FromNethernetId <id>` | 来源 NetherNet 会话 ID |
| 版本 | `--protocol <ver>` | 协议版本号 |
| | `--EngineVersion <ver>` | 引擎版本 |
| | `--PatchVersion <ver>` | 补丁版本 |
| | `--ExpandParams <params>` | 附加参数 |
| 皮肤 | `--skin_image_path <path>` | 皮肤图片文件路径 |
| | `--skin_model_path <path>` | 皮肤模型文件路径 |
| | `--operator_client_data <file>` | 指定客户端数据文件 |
| 调试 / 行为 | `--logger` | 启用日志 |
| | `--disout` | 出错即退出 |
| | `--script_mcp` | 启用 MCP 模式 |
| | `--no_request_chunk` | 不请求区块 |
| | `--g79` | 平台模式（g79） |
| | `--pause` | 暂停并等待输入 |

## 文档

- [插件开发文档](docs/插件开发文档.md) — 使用 Python 2.7 开发脚本插件：结构、API、事件、打包
- [构建项目文档](docs/构建项目文档.md) — 三平台从源码构建、交叉编译、故障排查
- [Wiki 首页](docs/Home.md) — 文档中心

## 许可证

第三方库请参阅各自目录下的许可证文件。
