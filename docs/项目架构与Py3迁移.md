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
│   └─ Python 运行时 (PythonRuntime: 内嵌 Python 3.12)          │
├─────────────────────────────────────────────────────────────┤
│  Python 3.12 (部署在 exe 旁 python312/)                       │
│   ├─ 框架 source/init.py (加载插件、RPC 反作弊校验)            │
│   ├─ 插件 scripts/<name>/__init__.py                         │
│   └─ 类型桩 python312/Lib/site-packages/*.pyi (IDE 补全)      │
└─────────────────────────────────────────────────────────────┘
```

## 2. Python 3.12 内嵌机制

### 初始化流程 (`PythonRuntime.cpp::startUp`)

1. `SetPythonHome()`:指向 `<exe目录>/python312`(通过 `GetModuleFileNameA` 计算)。
2. `Py_Initialize()` → `PyEval_SaveThread()`(释放 GIL 给工作线程)。
3. `PyGILState_Ensure()` → `initModules()`。
4. source 模式:设置 sys.path(追加 `./source`、`./scripts`、`./python312/Lib/site-packages`,**不清理**默认路径,否则标准库全丢)。
5. `import init`。

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
