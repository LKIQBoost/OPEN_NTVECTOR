# OPEN_NTVECTOR

_Modern Minecraft Bedrock protocol client, with an embedded Python 3.12 plugin runtime._
_基于网易我的世界基岩版协议的机器人客户端，内嵌 Python 3.12 插件运行时。_

[![stars](https://img.shields.io/github/stars/LKIQBoost/OPEN_NTVECTOR?style=flat-square)](https://github.com/LKIQBoost/OPEN_NTVECTOR/stargazers)
[![last commit](https://img.shields.io/github/last-commit/LKIQBoost/OPEN_NTVECTOR?style=flat-square)](https://github.com/LKIQBoost/OPEN_NTVECTOR/commits)
![platform](https://img.shields.io/badge/platform-Windows-blue?style=flat-square)
![C++](https://img.shields.io/badge/C%2B%2B-17-00599C?style=flat-square&logo=cplusplus&logoColor=white)
![Python](https://img.shields.io/badge/Python-3.12-3776AB?style=flat-square&logo=python&logoColor=white)

> 协议版本 **1.21.120**（`ProtocolVersion = 860`）
> · CPython **3.12.2** 静态链接进 `Program.exe`，运行时**不需要** `python312.dll`
> · 第三方依赖全部随仓库提供，构建**无需联网**

---

## Welcome

- OPEN_NTVECTOR is a Minecraft Bedrock Edition (NetEase) protocol client. C++ handles
  networking, login and packets; Python handles your logic.
- OPEN_NTVECTOR 是一个网易我的世界基岩版协议客户端。C++ 侧负责连接、登录、协议编解码与
  游戏状态维护，你只需要写 Python 插件。

```python
# scripts/my_plugin/my_plugin/__init__.py

def on_start(args):
    engine.message('', 'Hello, world!')

engine.register(on_start, "ModEventStartUp")
```

## Feature

- **开箱即用** —— 第三方 C/C++ 依赖（OpenSSL / libdatachannel / libwebsockets / SLikeNet /
  jsoncpp / zlib）与 CPython 源码全部随仓库提供，一条 `cmake` 命令从源码构建，无需包管理器。
- **自包含解释器** —— CPython 3.12.2 编成静态库直接链进 `Program.exe`，不依赖外部
  `python312.dll`，部署目录干净。
- **插件即时生效** —— source 模式下每 1.5 秒扫描 `scripts/`，改完保存即热重载，
  且只重载被修改的那个插件。
- **完整协议栈** —— 四种网络后端：SLikeNet（RakNet 魔改）· libwebsockets ·
  libdatachannel（WebRTC DataChannel）· NetherNet 直连。
- **加密与鉴权** —— AES-GCM / CTR / ECB、ChaCha20、Rotor、ECC / ECDSA 密钥交换、
  Base64，以及网易 RPC 校验链。
- **丰富的 Python 接口** —— 19 个 C++ 实现的原生模块（`engine` / `nbt` / `packets` /
  `game_state` / `pkt` / …），附带 `.pyi` 类型桩供 IDE 补全。

## Quick Start

**环境要求**：Windows · Visual Studio 2026（MSVC，需 `v145` 工具集）· CMake 4.x（最低 3.20）
· 一个 ≥ 3.10 的宿主 Python（仅 configure 期用于生成冻结模块，**不参与链接**）

```bash
git clone https://github.com/LKIQBoost/OPEN_NTVECTOR.git
cd OPEN_NTVECTOR

cmake -B build -G "Visual Studio 18 2026" -A x64
cmake --build build --config Release
```

产物：`build/application/Release/Program.exe`

> 首次 configure 会多花几分钟 —— 要编一遍静态 CPython 核心（先 `_freeze_module`
> 生成冻结模块，再编约 198 个核心源文件）。产物缓存在 `build/_deps/`，之后会跳过。

**部署**：把 `Program.exe` 与 `source/`、`scripts/` 放在同一目录。运行
`Program.exe --config mc.cfg --logger`。

> **首次使用**请先看文档 —— 部署目录布局、`mc.cfg` 配置项、命令行参数都有说明。
> 直接跑 `Program.exe` 不带参数会因缺少 `serverid` 而退出，这不是崩溃。

> 本项目是**非盈利**的开源项目，不解答协议逆向、注入、绕过风控等底层问题。
> 请遵守你所在服务器的规则；使用本项目的风险由使用者自行承担。

## Link

**Repository**

- 项目仓库：https://github.com/LKIQBoost/OPEN_NTVECTOR
- 构建产物：https://github.com/LKIQBoost/OPEN_NTVECTOR/releases

**Documentation**

<!--
  文档站点（VitePress）目前还没部署，所以这里不放链接。
  部署后把下面这行换成实际地址，例如：
  - 文档站点：https://lkiqboost.github.io/OPEN_NTVECTOR-docs/
-->
- 文档站点：**待部署**（VitePress 源码在独立项目 `OPEN_NTVECTOR-docs` 中）

## Thanks

本项目站在这些开源项目的肩上：

- [CPython](https://github.com/python/cpython) —— 内嵌的 Python 解释器
- [OpenSSL](https://github.com/openssl/openssl) —— TLS 与加密原语
- [libdatachannel](https://github.com/paullouisageneau/libdatachannel) —— WebRTC DataChannel
- [libwebsockets](https://github.com/warmcat/libwebsockets) —— WebSocket
- [SLikeNet](https://github.com/SLikeNet/slikenet) —— RakNet 魔改分支
- [jsoncpp](https://github.com/open-source-parsers/jsoncpp) · [zlib](https://github.com/madler/zlib) · [pybind11](https://github.com/pybind/pybind11)

也感谢所有写插件的人 —— 这个项目存在的意义就是让你们少碰 C++。

---

## License

1. 本项目**尚未声明自身许可证**，仓库根目录没有 `LICENSE` 文件。
2. 所有第三方库的许可证请参阅各自目录下的许可文件
   （`third_party/*`、`application/include/*`）。
3. 在项目明确许可证之前，请勿假定可以自由分发或商用。
