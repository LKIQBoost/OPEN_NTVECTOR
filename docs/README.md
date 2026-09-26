# NTVECTOR_Platform

网易我的世界基岩版机器人客户端（Vector），基于 C++17 与 CMake 构建，内嵌 **Python 3.12.2** 运行插件（已从 Python 2.7 完成迁移）。

主要开发/运行平台为 **Windows (MSVC)**；Linux x86_64 / Android (Termux) 曾支持（见 [docs/构建项目文档.md](docs/构建项目文档.md)）。
所有第三方 C/C++ 依赖均随仓库提供并从源码编译，构建时**无需外部包管理器、无需联网**。

## 主要特性

- **Minecraft 基岩版协议客户端**：登录 / 鉴权 / 数据包编解码
- **多种网络后端**：
  - SLikeNet（RakNet 魔改分支）— 联机通信
  - libwebsockets — WebSocket 协议
  - libdatachannel — WebRTC DataChannel（含 libjuice / libsrtp / usrsctp）
  - NetherNet 连接模式（`--start_nethernet`）
- **内嵌 Python 3.12.2 运行时**：注册 15 个内置模块（`engine` / `setting` / `mod_log` / `utility` / `pkt` / `aes` / `_chacha` / `_websocket` / `rotor` / `fop` / `_client` / `client_instance` / `easy_utils` / `_raknet` / `tan_lobby_game_clicpp_wrapper`，详见下方模块表）
- **插件热重载**：source 模式下每 1.5s 扫描 `scripts/` 变更，保存即生效
- **加密与鉴权**：AES-GCM / AES-CTR / AES-ECB、ChaCha20、Rotor、ECC / ECDSA 密钥交换、Base64
- **MCP 文件系统模式**：可由 `client_cfg.json` 中的 `no_launch_mcp` 字段控制（MCP 打包为遗留功能）
- **皮肤系统**：皮肤数据解析、皮肤 / 模型文件加载与转换

## 环境要求

| 平台 | 要求 |
|------|------|
| **Windows**（主平台） | Visual Studio 2026（MSVC，需 `v145` 工具集）、CMake 4.x、**宿主 Python ≥ 3.10**（仅 configure 期用） |
| **Linux x86_64** | 历史支持（clang / gcc）；Py3 迁移后暂以 Windows 为主，构建脚本待更新 |
| **Android ARM64** | 历史支持（Termux 原生编译）；同上待更新 |

> **不再需要本机 python.org 开发安装来链接** —— 解释器核心从仓库内的
> `third_party/Python-3.12.2` 编出。但 configure 期需要一个 ≥ 3.10 的宿主 Python 跑
> `Tools/build/deepfreeze.py` 生成冻结模块；未指定时自动探测，也可用
> `-DNTVECTOR_PY_HOST=<python.exe>` 显式指定。
> 工具集/SDK 可用 `-DNTVECTOR_PY_TOOLSET=v145`、`-DNTVECTOR_PY_WINSDK=10.0.26100.0` 覆盖；
> `-DNTVECTOR_PY_REBUILD=ON` 强制重建静态核心。
> 从 Windows 交叉编译 Linux x86_64 使用 `cmake/toolchains/linux-clang-x86_64.cmake`（clang + lld，需提供目标 sysroot）——迁移后尚未回归验证。

## 构建（Windows）

在项目根目录执行：

```bash
cmake -B build -G "Visual Studio 18 2026" -A x64
cmake --build build --config Release
```

- 生成器：`Visual Studio 18 2026`，架构 `x64`
- 产物：`build/application/Release/Program.exe`
- ⚠️ 构建坑：改 header 后 MSBuild 增量编译不会重编依赖它的 .cpp，需 `touch` 强制重编；构建行为异常时优先 `rm -rf build` 重新 configure（不要复用损坏的旧构建目录）。

### 部署

实际运行位置为 **`D:/Game/Minecraft/Vector/`**：

```
D:/Game/Minecraft/Vector/
├── Program.exe          ← CPython 3.12 核心静态链在里面，不需要 python312.dll
├── source/              ← 运行环境
│   ├── Lib/             ← 标准库（已去 __pycache__/test/tkinter）
│   │   └── site-packages/ ← 第三方库装这里；含 umsgpack.py、RC4.py、chacha/、raknet/、*.pyi 类型桩
│   ├── init.py          ← 框架：加载插件 / 处理 RPC 反作弊校验
│   ├── proton.py        ← 解析数据包
│   └── redirect.py      ← 遗留
├── scripts/             ← 插件（每个插件一个目录）
└── mc.cfg / client_cfg.json / skin_data.json / skins/ / songs/
```

> **没有 `python312.dll` / `python3.dll` / `python312/` 目录** —— 解释器核心静态链在
> `Program.exe` 里，与 Python 2.7 时代的形态一致（当年是静态 `python27.lib`）。
> 代价：官方 `.pyd` 动态扩展无法 drop-in，详见
> [项目架构与 Py3 迁移](项目架构与Py3迁移.md) 第 7 节。

> Linux / Android 的历史构建命令见 [docs/构建项目文档.md](docs/构建项目文档.md)。

## 项目结构

```
NTVECTOR_Platform/
├── CMakeLists.txt              # 根 CMake 配置
├── application/                # 主程序源码（目标名：Program）
│   ├── Core/ Crypto/ Network/ Packets/ NBT/ platform/   # 协议、网络、登录、加密等
│   ├── Python/                 # PythonRuntime.cpp（解释器初始化 / 模块注册）等
│   ├── PythonAPI/              # NBT、数据包、game_state 等 pybind11 绑定
│   └── include/                # SLikeNet、zlib、pybind11、OpenSSL
├── cmake/                      # FindPrebuiltDeps / BuildOpenSSL / toolchains 等
│   ├── BuildPython312.cmake    # configure 期编出静态 CPython 核心库
│   └── PythonBuiltinModules.cmake  # 编进 exe 的 stdlib C 扩展白名单
├── docs/                       # 文档（架构、构建、插件开发）
├── third_party/                # 第三方依赖源码（jsoncpp、libdatachannel、libwebsockets…）
│   └── Python-3.12.2/          # CPython 源码（用于静态核心；含 `PCbuild` 工程）
└── tools/                      # 辅助脚本与原生测试
```

## 依赖

所有 C/C++ 依赖均随仓库提供并从源码编译，构建过程不需要联网下载。

| 库 | 版本 | 构建方式 |
|----|------|----------|
| Python | 3.12.2 | 从 `third_party/Python-3.12.2` 编出**静态**核心库（`cmake/BuildPython312.cmake`，configure 期完成并缓存），链进 `Program.exe`；运行时无需 `python312.dll` |
| OpenSSL | 3.5.0 | cmake 配置期自动构建；Windows 失败时回退到项目自带 `.lib` |
| jsoncpp | 1.9.6 | `add_subdirectory` |
| libdatachannel | 最新 | `add_subdirectory`（含 libjuice、libsrtp、usrsctp） |
| libwebsockets | 4.5 | `add_subdirectory` |
| SLikeNet | 定制（RakNet 魔改） | `add_subdirectory` |
| zlib | 1.3.1 | `add_subdirectory` |

> 加第三方 Python 库：`pip install <pkg> --target "D:\Game\Minecraft\Vector\source\Lib\site-packages"`。
> 只支持 **纯 Python** wheel（`py3-none-any`）。带平台 tag 的 wheel（`numpy`/`Pillow` 那种
> `cp312-...-win_amd64`）不能用——原因见 [项目架构与 Py3 迁移](项目架构与Py3迁移.md) 第 7 节。

## 内嵌 Python 模块（15 个）

| 模块 | 用途 |
|------|------|
| `engine` | 事件 / 指令 / 坐标 / RPC |
| `setting` | 玩家 / 版本信息 |
| `mod_log` | 日志 |
| `utility` | 加解密 |
| `pkt` | 协议数据包读写 |
| `aes` | AES-128-ECB |
| `_chacha` | ChaCha20 |
| `_websocket` | WebSocket |
| `rotor` | Rotor 加密 |
| `fop` | MCP 文件（遗留） |
| `_client` | 客户端实例（遗留） |
| `client_instance` | pybind11 包装 |
| `easy_utils` | pybind11 加解密 |
| `_raknet` | RakNet（遗留） |
| `tan_lobby_game_clicpp_wrapper` | NetherNet（运行时注册） |

## 编进 exe 的 stdlib C 扩展

静态嵌入下没有 `.pyd` 可加载，CPython 3.12 拆出去的那批 stdlib C 扩展必须编进
`Program.exe` 并注册为 builtin（`cmake/PythonBuiltinModules.cmake` 里的白名单表，
加一行 + 重编即可增删）：

`_socket` `select` `_queue` `zlib` `_hashlib` `unicodedata` `_overlapped` `_zoneinfo`
`winsound` `_asyncio` `_uuid` `_multiprocessing` `pyexpat` `_elementtree`

其余大部分（`binascii` / `_struct` / `_json` / `_pickle` / `_random` / `_thread` /
`_sre` / `_csv` / `_datetime` / `math` / `cmath` / `array` / `mmap` / `signal` /
`_winapi` / `winreg` / `cjkcodecs` 等 74 个）本来就是核心内建。

`_bz2` / `_lzma` / `_sqlite3` / `_ctypes` / `_decimal` / `_tkinter` 需要额外引入
第三方源码（libbz2 / liblzma / sqlite3 / libffi / libmpdec+MASM / Tcl-Tk），
**不在**当前白名单内。

## 配置文件

| 文件 | 作用 |
|------|------|
| `mc.cfg` | 启动配置（JSON）：`room_info`（服务器 IP / 端口）、`player_info`（账号 / 昵称 / token）、`misc`（鉴权服务器地址、引擎版本、网易 SID 等）、`skin_info`（皮肤 / 模型路径） |
| `client_cfg.json` | 客户端配置（JSON）：`no_launch_mcp` 是否启用 MCP 文件系统模式（旧 `options.txt` 已并入） |
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

## 测试

- 运行：`D:/Game/Minecraft/Vector/Program.exe --logger`（配合启动参数，如 `--ServerIP` 等）
- 日志：`log.txt`（完整）、`dis.log`（断开 / 封禁）
- `PlayStatus value: 3` = **PlayerSpawn（正常进服）**，不是错误；`0` = 登录成功；`1/2/4/5/6` 才是失败
- 反作弊封禁"因违规游戏行为，您的账号被禁止进入游戏" = RPC/MCP 校验链有问题（`umsgpack.compatibility`、`triggerBytes` 二进制事件等，详见 [docs/项目架构与Py3迁移.md](docs/项目架构与Py3迁移.md)）
- 插件开发文档：`D:/Game/Minecraft/Vector/scripts/README.md`（Py3 版）
- 类型桩（IDE 补全 / 喂给 AI）：`source/Lib/site-packages/*.pyi`

## 文档

- [项目架构与 Py3 迁移](docs/项目架构与Py3迁移.md) — 架构详解、事件系统、反作弊校验链、Py2→Py3 迁移记录
- [构建项目文档](docs/构建项目文档.md) — 三平台从源码构建、交叉编译、故障排查
- [插件开发文档](docs/插件开发文档.md) — 使用 Python 开发脚本插件：结构、API、事件、打包
- [Wiki 首页](docs/Home.md) — 文档中心

## 许可证

第三方库请参阅各自目录下的许可证文件。
