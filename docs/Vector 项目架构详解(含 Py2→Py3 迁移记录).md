# Vector 项目架构详解(含 Py2→Py3 迁移记录)

> 面向接手开发的 AI/开发者。核心速查见根目录 `CLAUDE.md`,本文是深层架构。

---

## 1. 整体架构

```
┌─────────────────────────────────────────────────────────────┐
│  C++ 客户端 (application/)                                    │
│   ├─ 登录链 (LoginAuth/LoginSession/ConnectInstance)          │
│   ├─ 数据包 (PacketBase + 各 packet 类, 网易协议)              │
│   ├─ 回调分发 (CallbackManger: 包 → 触发 Python 事件)          │
│   └─ Python 运行时 (PythonRuntime: 静态链入的 CPython 3.12)   │
├─────────────────────────────────────────────────────────────┤
│  Python 3.12 (部署在 exe 旁 source/)                          │
│   ├─ 标准库 source/Lib/                                       │
│   ├─ 框架 source/init.py (加载插件、RPC 反作弊校验)            │
│   ├─ 插件 scripts/<name>/__init__.py                         │
│   └─ 类型桩 source/Lib/site-packages/*.pyi (IDE 补全)         │
└─────────────────────────────────────────────────────────────┘
```

> **解释器是静态链接进 Program.exe 的**,exe 旁**没有** `python312.dll`。这与 Python 2.7
> 时代的形态一致(当年是静态 `python27.lib`,32 MB)。代价是官方 `.pyd` 动态扩展无法加载
> —— 详见第 7 节。
```

## 2. Python 3.12 内嵌机制(静态链接)

解释器核心由 `cmake/BuildPython312.cmake` 在 configure 期从 `third_party/Python-3.12.2`
编成静态库 `python312core.lib`(~74 MB),直接链进 `Program.exe`。运行时**不需要**
`python312.dll` / `python3.dll` —— 与 Python 2.7 时代(静态 `python27.lib`)形态一致。

### 初始化流程 (`PythonRuntime.cpp::startUp`)

1. `SetPythonHome()`:指向 `<exe目录>/source`(通过 `GetModuleFileNameA` 计算)。
   标准库在 `source/Lib/`:`Modules/getpath.py:193-194` 在 Windows 上用 `Lib\os.py`
   作 landmark 在 prefix 下找标准库;**缺 `DLLs/` 目录是被显式容忍的**
   (`getpath.py:610-614` 注释 `If DLLs is missing on Windows, don't warn`),
   所以静态链接后删掉 `DLLs/` 不会报错。
2. `AppendStdlibInittab()`:**必须在 `Py_Initialize()` 之前**,它把编进 exe 的
   stdlib C 扩展注册进 builtin import table。声明与注册代码由
   `cmake/PythonBuiltinModules.cmake` 生成到
   `build/generated/ntvector_python_builtin_inittab.inc`。
3. `Py_Initialize()` → `PyEval_SaveThread()`(释放 GIL 给工作线程)。
4. `PyGILState_Ensure()` → `initModules()`。
5. source 模式:设置 sys.path(追加 `./source`、`./scripts`、`./source/Lib/site-packages`,
   **不清理**默认路径,否则标准库全丢)。
6. `import init`。

> **为什么用 `PyImport_AppendInittab` 而不是往 `sys.modules` 塞**:前者是
> `Python/import.c` 契约里的 pre-init-only 接口(`PyImport_AppendInittab() may not be
> called after Py_Initialize()`),它决定 `sys.builtin_module_names`,也是
> `BuiltinImporter`(`sys.meta_path` 第一顺位)的查找来源;后者只对 `sys.modules`
> 已命中的 `import` 生效,`find_spec`、子解释器、以及 `initModules()` 之前的 import
> 都看不到。本工程自己的 19 个模块仍走 `sys.modules`(见 `initModules`),不动。

### 编进 exe 的 stdlib C 扩展(白名单)

CPython 3.12 把 `_socket` / `select` / `_queue` 等一批 stdlib C 模块从核心拆成了独立
`.pyd`。静态链接下这些文件加载不了(见第 7 节),必须把源码编进 exe 并注册为 builtin。

白名单是 `cmake/PythonBuiltinModules.cmake` 里的一张表,**加一个扩展 = 表里加一行 +
重编 exe**,C++ 侧不用改:

| 模块 | 备注 |
|------|------|
| `_socket` `select` `_queue` | 框架/插件实际用到:`Lib/socket.py:52` 与 `Lib/selectors.py:12` 是**硬 import** |
| `zlib` | 核心出于符号冲突没内建它;对着项目自己的 `zlibstatic` 编 |
| `_hashlib` | 源文件是 `Modules/_hashopenssl.c`(不是 `_hashlib.c`) |
| `unicodedata` `_overlapped` `_zoneinfo` `winsound` `_asyncio` `_uuid` `_multiprocessing` | 常见插件写法覆盖 |
| `pyexpat` `_elementtree` | 共用 `Modules/expat/` 的三个 `.c`(内置,无需外部 expat) |

**已经是核心内建、不要重复加的**:`winreg`(`PC/winreg.c`)、`zlib` 的实现、
`binascii`/`_struct`/`_json`/`_pickle`/`_random`/`_thread`/`_sre`/`_csv`/`_datetime`/
`math`/`cmath`/`array`/`mmap`/`signal`/`_winapi`/`cjkcodecs`(gbk/gb18030 等)等 74 个模块。

**需要额外引入第三方源码才能加**:`_bz2`(libbz2)、`_lzma`(liblzma)、`_sqlite3`
(sqlite3)、`_ctypes`(libffi)、`_decimal`(除 libmpdec 外还要 MASM 汇编
`vcdiv64.asm`)、`_tkinter`(Tcl/Tk,且需要运行时脚本库)。这些**不在**本期白名单内。

`hashlib` 不需要 `_hashlib` 也能用:`Lib/hashlib.py:175-178` 有
`except ImportError: __get_hash = __get_builtin_constructor`,回退到内建
`_sha256`/`_sha512`/`md5`。只有 `pbkdf2_hmac`/`scrypt` 依赖 OpenSSL 版。

### 模块注册 (`PythonRuntime.cpp::initModules`)

每个模块用 `PyModuleDef + PyModule_Create`,然后 `RegisterModule()` 放进 `sys.modules`:

| C++ 文件 | 模块名 | 用途 | init 函数 |
|----------|--------|------|-----------|
| engine.cpp | `engine` | 事件/指令/坐标/RPC | `PyInit_engine` |
| engine.cpp | `_client` | 客户端实例(遗留) | `PyInit__client` |
| engine.cpp | `client_instance` | pybind11 包装 | `PyInit_client_instance` |
| PythonEasyUtilsWrapper.h | `easy_utils` | pybind11 加解密 | `PyInit_easy_utils` |
| TanGame.cpp | `tan_lobby_game_clicpp_wrapper` | NetherNet(运行时注册) | `register_tan_lobby_game_module` |
| setting.cpp | `setting` | 玩家/版本信息 | `PyInit_setting` |
| mod_log.cpp | `mod_log` | 日志 | `PyInit_mod_log` |
| utility.cpp | `utility` | 加解密 | `PyInit_utility` |
| pkt_module.cpp | `pkt` | 协议数据包读写 | `PyInit_pkt` |
| fopmodule.cpp | `fop` | MCP 文件(遗留) | `PyInit_fop`(static,被 .h include) |
| rotormodule.c | `rotor` | Rotor 加密 | `PyInit_rotor`(static,被 .h include) |
| aes_ecb_wrapper.cpp | `aes` | AES-128-ECB | `PyInit_aes` |
| chacha_wrapper.cpp | `_chacha` | ChaCha20 | `PyInit__chacha` |
| websocket_wrapper.cpp | `_websocket` | WebSocket | `PyInit__websocket` |
| raknet_wrapper.cpp | `_raknet` | RakNet(遗留) | `PyInit__raknet` |

> **坑**:`fopmodule.cpp`/`rotormodule.c` 通过 `.h` 里 `#include "xxx.cpp"` 编译进多个 TU,init 函数必须 `static`(否则链接重复定义)。

### 字符串编码边界

- **C++ → Python**:文本用 `PyTextFromUtf8`(UTF-8 数据)/ `PyTextFromGbk`(GBK 配置,走 Python 的 gbk codec);二进制用 `PyBytes_FromStringAndSize`。
- **Python → C++**:`s`/`s#` 在 Py3 给出 **UTF-8**;`y#` 给出 **bytes**。
- 集中辅助在 `Py3Compat.h`(header-only,无 windows.h 依赖)。

## 3. 事件系统

### C++ → Python (`engine_wrapper.h` PythonEventEngine)

- `trigger(event, args...)`:模板,自动转 C++ 类型为 Python 对象,包成 list。
- `triggerBytes(event, std::string binary)`:**必须用这个传二进制事件**(如 on_rpc),它把 bytes 包成 `[bytes]`。**直接 trigger 传 std::string 会当 UTF-8 文本解码**。
- `engine.cpp::trigger_event`:要求 args 是 **list/tuple**,否则静默丢弃(handler 不触发)。

### Python 注册

- 用户事件:`engine.register(cb, "事件名")`。
- 内核事件:`engine.register_kernel_event(cb, "事件名")`(仅 init.py)。
- 协议事件:`engine.register_protocol_event(id, cb, False)`。
- 回调签名固定:`def handler(args)`,args 是 list。

## 4. 关键数据包实现

### CommandBlockUpdate (ID 78, 客户端→服务端)

**必须带 `filtered_name`**(1.21.120 协议,在 `name` 之后、`should_track_output` 之前)。缺失 → 服务端解析越界 → 踢。字段顺序:
```
is_block:bool
[if block] position(UBlockPos: x=zigzag32, y=varint, z=zigzag32), mode:varint, needs_redstone:bool, conditional:bool
[else] minecart_entity_runtime_id:varint64
command:string, last_output:string, name:string, filtered_name:string, should_track_output:bool, tick_delay:li32, execute_on_first_tick:bool
```
协议版本:`ProtocolVersion=860` = **1.21.120**。

### SettingsCommand (ID 140)

- 序列化:`WriteUInt8(ID())` + `WriteUInt8(1)` + `WriteVarUInt(len)` + command + suppress。ID 140 的 VarUInt 编码恰好是 `[0x8C][0x01]`,与硬编码的 "1" 字节巧合对齐。
- **普通用户别用**(反作弊风险),用 `engine.command()`。

## 5. 框架 RPC / 反作弊校验 (source/init.py)

服务器通过 `PyRpc` 包发 `on_rpc` 事件,框架响应:

```
S2CHeartBeat     → 回 ClientLoadAddonsFinishedFromGac
GetStartType     → 用 utility 解密/加密,回 SetStartType
GetMCPCheckNum   → 计算 SetMCPCheckNum(反作弊校验)
```

**MCP 校验链**(Py2→Py3 全是坑):
- `_mcp_rotor_decrypt`:rotor.decrypt + zlib + `_mcp_reverse_data`。
- `_get_calc_check_num`:RC4 解密 + md5。
- **必须** `umsgpack.compatibility = True`(否则 >31 字节字符串打 str8 而非 raw16)。
- RC4/`_mcp_reverse_data`/`_get_calc_check_num` 已改 Py3(bytes/int 操作)。

校验失败 → 服务器判定"违规游戏行为"→ 踢。

## 6. Py2 → Py3 迁移记录(本次会话完成)

| 阶段 | 内容 |
|------|------|
| M0 | Python 3.12 工具链验证(链接/嵌入/DLL 部署) |
| M1 | 简单模块迁移(mod_log/setting/utility/aes/websocket/chacha) |
| M2 | 中型模块(pkt/fop/raknet/rotor) |
| M3 | 引擎核心(engine_wrapper.h + engine.cpp + pybind11 2.13) |
| M4 | 运行时(PythonRuntime + PythonUtils + CMake 切换) |
| M5 | Python 侧(source/ 框架 + RC4/umsgpack 迁移到 site-packages) |
| M6 | 部署(embeddable → python312/ 完整标准库) + 回归 |

**最终部署**:用用户本机 Python 3.12.2 的 Lib(裁剪到 27MB)+ DLLs,放 `python312/`;`umsgpack.py`/`RC4.py`/`chacha/`/`raknet/` 移到 site-packages;`source/lib`(旧 Py2 标准库)已删除。

**主要修复的问题**(全部记录在 CLAUDE.md 的"关键坑"):
- `PY_SSIZE_T_CLEAN`、msgpack compatibility、triggerBytes list 参数、MSBuild 陈旧头文件、`GetStartType` str+bytes、RC4/umsgpack bytes、`execfile`、`import thread`→`_thread`、`reload`→`importlib.reload`、`CommandBlockUpdate filtered_name`、`byte` 歧义(禁用 std::byte 或避免 windows.h)、pybind11 2.13 `create_extension_module(nullptr)` 崩溃等。

---

## 7. 静态链接模式与扩展限制

### 形态

解释器核心由 `cmake/BuildPython312.cmake` 在 configure 期编成静态库
`python312core.lib`(~74 MB)链进 `Program.exe`,exe 旁**没有** `python312.dll`。
这与 Python 2.7 时代一致:当年用 `Py_NO_ENABLE_SHARED` + `PYTHON_STATIC` 链
32 MB 的静态 `python27.lib`,由仓库里 `Python-2.7.18/PCbuild` 的 `pythoncore` 工程
(MSBuild,amd64 Release)产出。

CPython 官方在 3.12 把 `PCbuild/pythoncore.vcxproj` 改成直接产 DLL 了,所以
`BuildPython312.cmake` 会读取该工程、打 7 处补丁(StaticLibrary / TargetName /
去掉 `_TriggerRegen`+`regen.targets` / 去掉 `python_nt.rc` / 补 zlib include /
相对路径与 Import 绝对化),生成副本到 `build/_deps/python-static/proj/` 再构建,
**不改 third_party 里的源码**。

构建分两步:`_freeze_module` 先生成 `Python/frozen_modules/*.h` 与
`Python/deepfreeze/deepfreeze.c`(python.org 的源码 tarball 里这两个都是空的,
只有 README.txt),然后才编核心。产物与指纹缓存在 `build/_deps/python-static/`,
`-DNTVECTOR_PY_REBUILD=ON` 可强制重建。

### 链接宏怎么分配(搞反的后果很隐蔽)

| 构建单元 | 宏 | 效果 |
|---|---|---|
| `python312core.lib` | **保留** `Py_ENABLE_SHARED` + `Py_BUILD_CORE` | `Py_EXPORTED_SYMBOL` 非空 → API 带 `dllexport` 导出进 Program.exe |
| `Program.exe` 的应用 TU | `Py_NO_ENABLE_SHARED`(由 `python312core` 的 INTERFACE 提供) | API 声明不是 `dllimport`,同映像内直接调用 |
| 用户自编 `.pyd` | 默认(`pyconfig.h` 自动给 `Py_ENABLE_SHARED`) | 按模块名从 `Program.exe` 导入 |

依据是 `Include/exports.h:6-9` —— **`Py_ENABLE_SHARED` 没有定义时
`Py_EXPORTED_SYMBOL` 是空的**,`Include/pyport.h:507` 的整个 dllexport 分支也以它为门:
核心若写成 `Py_NO_ENABLE_SHARED` 会得到**零导出**。反过来应用侧若带
`Py_ENABLE_SHARED`,`PyAPI_FUNC` 就变成 `dllimport`,会去找一个并不存在的
`python312.dll` 而链接失败。

`application/application.cpp` 顶部原来那句 `#define Py_ENABLE_SHARED` 已删除 ——
链接方式必须只由 CMake 决定,否则该 TU 会和其他 TU 取到不同的 `pyconfig.h` 分支。

pybind11 的 `PYBIND11_EXPORT` 默认给 `PyInit_xxx` 挂 `__declspec(dllexport)`,而
静态侧 `PyMODINIT_FUNC` 不带,属性不一致会触发 MSVC `C2375 重定义;不同的链接`;
工程里把 `PYBIND11_EXPORT` 置空对齐(pybind11 用 `#if !defined(...)` 保护,可以覆盖)。

### 限制一:官方 `.pyd` 不能 drop-in

那批官方 `.pyd` 加载不了。`.pyd` 是按**模块名**从 `python312.dll` 导入符号的,静态链接
后核心在 `Program.exe` 里;即便名字能对上,`.pyd` 自己去加载一份 `python312.dll` 也会
变成**两个运行时**(`Py_None`、`_PyRuntime`、类型对象的数据段分裂),必然崩溃或类型错乱。

- **能用**:`py3-none-any` 纯 Python wheel(`requests` / `websockets` / `pyserial` / …)
- **不能用**:`cp312-cp312-win_amd64` 平台 wheel(`numpy` / `Pillow` / `lxml` / …)

装库:`pip install <pkg> --target "<Vector>\source\Lib\site-packages"`。

### 限制二:`Program.exe` 不能改名

exe 会导出约 1700 个 CPython API 符号并产出 `Program.lib`,供用户自编的 `.pyd` 从
**已加载的 exe** 导入符号。这个绑定靠 PE 导入表里的模块名,改名后 `.pyd` 会加载失败。

### 限制三:加 C 扩展要重编

- 常用扩展:改 `cmake/PythonBuiltinModules.cmake` 的表 + 重编 exe
- 用户自己的 C 扩展:对着 `Program.lib` 编成 `.pyd`,放进 `source/Lib/site-packages/`

### 已验证 / 未验证

**已实测**(本机 `Program.exe` + 全新部署目录):
- `dumpbin /dependents` 无 `python312.dll` / `python3.dll`
- `sys.prefix` 指向 `<exe>\source`,`sys.path` 含 `source\Lib`
- `import init` 成功,框架注册了 `on_rpc` / `mcp_compile` 等 handler
- 14 个编进 exe 的扩展全部出现在 `sys.builtin_module_names`
- `pip install requests --target source\Lib\site-packages` 后 `import requests` 可用

**未验证**:
- **实际连服回归**(`PlayStatus value: 3` = PlayerSpawn 正常进服;以及未被反作弊踢)
  —— 静态化动了初始化路径,MCP 校验链(rotor + umsgpack + RC4)必须在有账号/服务器的
  环境下实测。这是上线前的硬门槛。
- 「用户自编 `.pyd` 从 exe 导入符号」这条路的可行性只有 `dumpbin /exports` 佐证:
  我方导出 1693 个符号(含数据符号 `_Py_NoneStruct` / `PyExc_TypeError`),官方
  `python312.dll` 是 1696 个,只差 `Py_Main` / `Py_RunMain` / `Py_BytesMain` 三个
  控制台入口(静态库按需拉取对象,`main.c` 没被引用)。尚未用真实 `.pyd` 端到端跑过。

