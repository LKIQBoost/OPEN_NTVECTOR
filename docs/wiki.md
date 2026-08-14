# Vector 我的世界基岩版机器人插件开发文档

> 整合版 — 覆盖插件结构、API、事件、打包、调试全流程
> **当前环境：Python 3.12.2（已从 Python 2.7 迁移）**

---

## 目录

1. [基础环境](#1-基础环境)
2. [插件目录结构](#2-插件目录结构)
3. [插件生命周期与热重载](#3-插件生命周期与热重载)
4. [事件系统](#4-事件系统)
5. [engine API 总览](#5-engine-api-总览)
6. [模块与工具](#6-模块与工具)
7. [协议数据包处理](#7-协议数据包处理)
8. [线程与资源管理](#8-线程与资源管理)
9. [MCP 打包发布（遗留）](#9-mcp-打包发布遗留)
10. [代码模板](#10-代码模板)
11. [常见问题](#11-常见问题)

---

## 1. 基础环境

| 项目 | 说明 |
|------|------|
| Python 版本 | **3.12.2**（Vector 内置，位于 `python312/` 目录） |
| 语法 | 使用 **Python 3** 语法 |
| 开发模式 | **source 模式**（源码）— 修改 `.py` 文件后自动热重载 |
| 发布模式 | MCP 模式（遗留，见第 9 节，**仅 source 模式受支持**） |
| Python 运行时 | `python312/`（Lib 标准库 + DLLs + site-packages），`python312.dll` 在 exe 旁 |
| 插件目录 | `scripts/` — 所有插件均放在此目录下 |

> **不要**修改 `source/` 下的文件，那是 Vector 运行环境（框架脚本）。

### 第三方库

Vector 内置完整 Python 3.12 标准库。如需第三方库（`requests`、`numpy` 等）：

```bash
# 用系统 Python 的 pip 安装到 Vector 的 site-packages：
pip install requests --target "Vector\python312\Lib\site-packages"
```

或直接把纯 Python 包复制到 `python312/Lib/site-packages/`。插件中直接 `import` 即可。

---

## 2. 插件目录结构

```
scripts/
  my_plugin/                  ← 插件根目录（与内层包名一致）
    redirect.py               ← MCP 导入钩子（source 模式可为空文件）
    my_plugin/                ← 实际 Python 包
      __init__.py             ← 插件入口（必须，加载时自动执行）
      other_module.py         ← 其他模块（可选）
      utils/                  ← 子包（可选）
        __init__.py
```

**命名规则：**

- 插件根目录名 = 内层包名 = 插件名，三者**必须一致**
- `__init__.py` 是插件入口，加载时自动执行
- `redirect.py` 只在 MCP 模式下生效，source 模式可以放空文件
- **不要在**插件根目录放 `__init__.py`，否则会和内层包冲突

---

## 3. 插件生命周期与热重载

### 加载流程

```
start_game 事件
  → init.py 调用 load_all_plugins()
  → 扫描 scripts/ 下的插件目录
  → 将插件根目录加入 sys.path
  → __import__('插件名') 执行 __init__.py
  → 触发 ModEventStartUp 事件（所有插件加载完成）
```

### 热重载

**开发阶段（source 模式）自带文件监视：**

- 每 1.5 秒扫描 `scripts/` 下所有 `.py` 文件的修改时间
- **只重载有修改的那个插件**，不影响其他插件
- 保存文件即生效，无需手动操作

**单插件重载流程：**

```
1. 调用插件的 on_uninit()（如果实现了）
2. 精准注销该插件注册的所有事件
3. importlib.reload() 重新执行插件代码
```

**全量重载：**

```python
engine.trigger("reload")       # 或 engine.trigger("mod_reload")
```

全量重载流程：

```
1. 触发 ModEventUnInit 事件（所有插件收到）
2. engine.cleanup_user() 清除所有用户事件
3. engine.clear_all_user_protocol_event() 清除所有用户协议事件
4. 逐个 reload() 所有已加载插件
5. 触发 ModEventStartUp 事件
```

也可以在 Python 层调用：

```python
import init
init.reload_plugin("my_plugin")      # 重载指定插件
init.reload_all_plugins()            # 重载所有插件
```

---

## 4. 事件系统

### 注册规则（必须遵守）

| 规则 | 说明 |
|------|------|
| `engine.register(cb, "事件名")` | **唯一合法**的用户事件注册方式 |
| ~~`engine.register_kernel_event()`~~ | **禁止使用**，仅 `init.py` 可用 |
| ~~`engine.register_protocol_event(id, cb, True)`~~ | **禁止使用**（kernel 协议事件），仅 `proton.py` 可用 |
| `engine.register_protocol_event(id, cb, False)` | ✅ 注册用户协议事件，重载时自动清除 |

### engine.trigger() 参数规则

```python
engine.trigger("事件名")              # 无参数，回调收到 []
engine.trigger("事件名", [a, b])      # 回调收到 [a, b]
engine.trigger("事件名", some_obj)    # 非 list/tuple 会被包成 [some_obj]
```

### 系统内置事件

| 事件 | 触发时机 | 说明 |
|------|----------|------|
| `ModEventStartUp` | 所有插件加载完成后 | 适合做初始化 |
| `ModEventUnInit` | 全量重载前 | 应在此停止后台线程 |
| `on_respawn` | 玩家死亡后 | — |
| `on_move_player` | `/tp` 传送自身后 | 参数：`[x, y, z]` 坐标列表 |
| `on_command_output` | 发送指令后（proton.py 触发） | 参数：`[返回值, GUID]`，返回值 = `[[键, [参数...]], ...]` |
| `on_player_list` | 进入服务器后 | 参数：`[玩家列表]` |
| `on_text` | 收到聊天栏消息后（proton.py 触发） | 参数：`[source_name, message, msg_type, parameters]` |
| `on_disconnect` | 与服务器断开连接后 | — |
| `on_rpc` | 收到远程调用后 | 内核事件，`init.py` 已处理 |

### OnTextPacket（proton.py 提供的高层事件）

聊天/系统消息，回调收到 `[dict]`：

```python
def on_text(args):
    data = args[0]
    # data 字段：type, needs_translation, source_name, message,
    #           parameters, xuid, platform_chat_id, filtered_message
    # 注意：source_name / message 已经是 str（Unicode），可直接使用
```

同时，proton.py 还会触发 `on_text` 事件，参数为位置列表 `[source_name, message, msg_type, parameters]`（与 `OnTextPacket` 的 `[dict]` 不同）。

**TextPacket type 值：**

| 值 | 常量 | 说明 |
|----|------|------|
| 0 | RAW | 原始文本 |
| 1 | CHAT | 聊天消息 |
| 2 | TRANSLATION | 翻译文本 |
| 3 | POPUP | 弹窗 |
| 4 | JUKEBOX | 唱片机 |
| 5 | TIP | 提示 |
| 6 | SYSTEM | 系统消息 |
| 7 | WHISPER | 私信 |
| 8 | ANNOUNCEMENT | 公告 |
| 9 | OBJECT | JSON 对象文本 |
| 10 | COMMAND | 指令反馈（message 为指令键名，parameters 为指令参数列表） |

> **type=10 (COMMAND)**：message 为指令键名（如 `commands.give.success`），`parameters` 为指令参数列表，可用于本地化指令反馈文案。

### 插件间通信

```python
# 插件 A 发送
engine.trigger("PluginA_Ready", [some_data])

# 插件 B 接收
engine.register(handler, "PluginA_Ready")
```

---

## 5. engine API 总览

### 事件注册与触发

| 函数 | 说明 |
|------|------|
| `engine.register(cb, event_name)` | 注册事件处理器 |
| `engine.unregister(event_name, cb)` | 注销指定事件 |
| `engine.trigger(event_name [, args])` | 触发事件（同步） |
| `engine.register_protocol_event(id, cb, False)` | 注册用户协议事件 |

### 游戏交互

| 函数 | 说明 |
|------|------|
| `engine.message(text)` | 发送聊天消息（单参数版本） |
| `engine.message(sender, msg)` | 发送聊天消息（sender 留空即可，**msg 直接支持中文**） |
| `engine.command(cmd_str)` | 发送普通游戏指令 |
| `engine.settingscommand(cmd_str)` | 发送 setting 格式指令（需要管理员/控制台权限），**优先使用此函数** |
| `engine.command_guid(cmd_str, guid)` | 发送指令并附带 16 字节 GUID（用于匹配返回） |
| `engine.move(x, y, z)` | 传送玩家到指定坐标 |
| `engine.respawn()` | 复活玩家（Vector 已自动复活，一般无需调用） |
| `engine.send(binary_data)` | 发送原始二进制数据包 |
| `engine.command_update(...)` | 更新命令方块数据（详见下方「命令方块更新」） |
| `engine.openContainer(x, y, z)` | 打开容器并返回缓存方块数据（详见下方「打开容器」） |

### 坐标与视角

| 函数 | 说明 |
|------|------|
| `engine.get_pot_x()` → `float` | 当前 X 坐标 |
| `engine.get_pot_y()` → `float` | 当前 Y 坐标 |
| `engine.get_pot_z()` → `float` | 当前 Z 坐标 |
| `engine.add_pot_x(delta)` | X 坐标偏移 |
| `engine.add_pot_y(delta)` | Y 坐标偏移 |
| `engine.add_pot_z(delta)` | Z 坐标偏移 |
| `engine.get_head_x()` → `float` | 头部俯仰角 |
| `engine.get_head_y()` → `float` | 头部偏转角 |
| `engine.add_head_x(delta)` | 增加俯仰角 |
| `engine.add_head_y(delta)` | 增加偏转角 |

### 服务器 / 进程信息

| 函数 | 说明 |
|------|------|
| `engine.get_server_ip()` → `str` | 服务器 IP |
| `engine.get_server_port()` → `int` | 服务器端口 |
| `engine.get_entity_runtime_id()` → `int` | 玩家实体运行时 ID |
| `engine.getparams()` → `list` | 进程启动参数列表 |

### 权限与配置

| 函数 | 说明 |
|------|------|
| `engine.disabled_auth_input()` | 启用 authinput（每 tick 发送验证包） |
| `engine.enable_auth_input()` | 关闭 authinput |
| `engine.get_auth_input()` → `bool` | 获取当前 authinput 状态 |
| `engine.get_mcp_load_config()` → `bool` | 返回当前模式：MCP / 源代码 |

### 系统工具

| 函数 | 说明 |
|------|------|
| `engine.system(cmd)` | 执行 Windows cmd 指令 |
| `engine.exit(code)` | 退出进程 |
| `engine.rpc(data)` | 发送 RPC（msgpack bytes） |

### 命令方块更新（command_update）

更新命令方块数据（MCBE 1.21.120 协议，**已可用**）：

```python
engine.command_update(x, y, z, command_block_type, redstone, conditional, delay, execute_on_first_tick, command, name)
```

参数说明：

| 参数 | 类型 | 说明 |
|------|------|------|
| `x, y, z` | `float` | 命令方块坐标 |
| `command_block_type` | `int` | 命令方块类型（0=普通、1=循环、2=脉冲） |
| `redstone` | `bool` | 是否受红石信号触发 |
| `conditional` | `bool` | 是否为条件命令方块（有条） |
| `delay` | `int` | 延迟 tick |
| `execute_on_first_tick` | `bool` | 是否首个 tick 立即执行 |
| `command` | `str` | 命令方块内执行的指令 |
| `name` | `str` | 命令方块名称 |

示例：

```python
# 在坐标 (106, 93, 69) 放置循环命令方块（type=1），执行 /say
engine.command_update(106, 93, 69, 1, False, False, 0, True, "/say VectorCommandUpdate", "Vector")
```

> 需要先确保该坐标有命令方块（可用 `setblock` 放置），且玩家有操作权限。

### 打开容器（openContainer）

打开指定坐标的容器（箱子、桶、熔炉等），并返回该方块已缓存的方块数据（MCBE 1.21.120 协议）：

```python
data = engine.openContainer(x, y, z)
```

参数说明：

| 参数 | 类型 | 说明 |
|------|------|------|
| `x, y, z` | `int` | 容器方块坐标 |

返回 `dict`：

```python
{
    "x": 106, "y": 93, "z": 69,
    "success": True,
    # 该坐标已缓存的方块实体数据（告示牌文本、命令方块命令等），未缓存为 None
    "block_data": {
        "nbt": {"id": "Chest", "CustomName": "..."},   # 已展开的 NBT 扁平字典
        "nbt_len": 123,                                # 原始 NBT 字节数
        "raw_nbt": "0a000b..."                         # 原始 NBT 十六进制
    } | None,
    # 该坐标容器的已缓存内容（打开过才会缓存），未缓存为 None
    "container": {
        "window_id": 2,
        "window_type": 0,                            # WindowType 枚举（0=普通容器）
        "entity_unique_id": 0,
        "slots": [
            {"network_id": 276, "count": 1, "metadata": 0,
             "block_runtime_id": 0, "has_stack_id": False, "stack_id": 0,
             "extra_hex": ""}
        ]
    } | None
}
```

说明：

- 函数发送 `InventoryTransactionPacket`（use_item / click_block）请求打开容器。
- 服务端随后推送 `ContainerOpenPacket`(46) + `InventoryContentPacket`(49)，客户端会**自动解析并缓存**；
  容器内容到达后再次调用 `openContainer` 即可在 `container.slots` 中拿到完整槽位。
- 方块实体数据（告示牌、命令方块）由 `BlockActorDataPacket`(56) 推送时自动缓存到 `block_data`。
- 异步获取原始数据包：用 `engine.register_protocol_event(46, cb, False)` / `(49, ...)` / `(56, ...)` 注册回调，
  回调参数为数据包原始字节（不含包 ID），可在 Python 侧自行解析。

> 需要玩家靠近容器且服务端允许交互；`container` 内容依赖服务端下发，首次调用时通常为 `None`。

---

## 6. 模块与工具

### C++ 内置模块（直接 import）

| 模块 | 用途 |
|------|------|
| `engine` | 事件系统、RPC、游戏控制 |
| `mod_log` | 日志输出到 C++ Logger |
| `setting` | 获取 UID、玩家 ID、版本号 |
| `utility` | 加解密工具 |
| `pkt` | 二进制数据包快速读写（MC 协议专用） |
| `fop` | MCP 文件系统操作（普通用户无需使用） |
| `rotor` | Rotor 加密 |
| `aes` | AES 加密 |
| `_chacha` | ChaCha20 加密 |
| `_websocket` | WebSocket 客户端 |

### mod_log — 日志

```python
import mod_log
mod_log.log(1, "普通信息")    # LOG_INFO  = 1
mod_log.log(2, "警告信息")    # LOG_WARN  = 2
mod_log.log(4, "错误信息")    # LOG_ERROR = 4
```

### setting — 玩家 / 版本信息

| 函数 | 返回值 | 说明 |
|------|--------|------|
| `setting.gettoken()` | `str` | 当前登录 token |
| `setting.getplayerid()` | `str` | Py 层玩家实体 ID |
| `setting.get_engine_version()` | `str` | 引擎版本 |
| `setting.get_patch_version()` | `str` | 补丁版本 |
| `setting.get_uid()` | `str` | 用户 ID |
| `setting.get_name()` | `str` | 玩家昵称（**已是 str，中文直接可用**） |

### utility — 加解密

| 函数 | 说明 |
|------|------|
| `utility.encrypt_with_tail(bytes)` → `bytes` | 加密（同 `/authentication-v2` 模式） |
| `utility.decrypt_with_tail(bytes)` → `bytes` | 解密（同 `/authentication-v2` 模式） |
| `utility.get_encrypt_token(token, body, url)` → `bytes` | 生成 HTTP 动态 token（用于 user-token 标头） |

### pkt — MC 协议数据包读写

**读取函数** — 所有读取函数返回 `(value, new_offset)`：

| 函数 | 说明 |
|------|------|
| `pkt.read_byte(data, off)` | uint8 |
| `pkt.read_bool(data, off)` | bool |
| `pkt.read_varint(data, off)` | 无符号 varint |
| `pkt.read_svarint(data, off)` | 有符号 zigzag varint |
| `pkt.read_string(data, off)` | varint 长度前缀字符串（**返回 str，已解码 UTF-8**） |
| `pkt.read_u16(data, off)` | 小端 uint16 |
| `pkt.read_u32(data, off)` | 小端 uint32 |
| `pkt.read_u64(data, off)` | 小端 uint64 |
| `pkt.read_i32(data, off)` | 小端 int32 |
| `pkt.read_i64(data, off)` | 小端 int64 |
| `pkt.read_f32(data, off)` | 小端 float32 |
| `pkt.read_f64(data, off)` | 小端 float64 |
| `pkt.read_bytes(data, off, n)` | 读 n 个字节（**返回 bytes**） |
| `pkt.remaining(data, off)` | 剩余字节数 |

> **Py3 行为变化**：`read_string` / `unpack 's'` 返回**解码后的 `str`**（Unicode），不再是 Py2 的 bytes。插件直接使用即可，中文无需再 `.decode()`。

**写入函数** — 所有写入函数返回 `bytes`：

| 函数 | 说明 |
|------|------|
| `pkt.write_byte(val)` | uint8 |
| `pkt.write_bool(val)` | bool |
| `pkt.write_varint(val)` | 无符号 varint |
| `pkt.write_string(s)` | varint 长度前缀字符串（接受 str） |
| `pkt.write_u16(val)` | 小端 uint16 |
| `pkt.write_u32(val)` | 小端 uint32 |
| `pkt.write_i32(val)` | 小端 int32 |
| `pkt.write_u64(val)` | 小端 uint64 |
| `pkt.write_f32(val)` | 小端 float32 |
| `pkt.write_f64(val)` | 小端 float64 |

**批量读取 / 写入：**

```python
# 批量读取
pkt.unpack("b?vs", data, off)       # → (val1, val2, val3, val4, new_off)

# 格式码
# b = byte(u8)    ? = bool          v = varint(无符号)
# V = svarint     s = varint前缀字符串 → 返回 str
# h = u16         H = i16           i = u32
# I = i32         q = u64           Q = i64
# f = f32         d = f64

# 批量写入
pkt.pack("b?vs", 1, True, 100, "hello")   # → bytes
```

---

## 7. 协议数据包处理

### 处理聊天消息（OnTextPacket）

`proton.py` 已注册 `TextPacket(0x09)` 的内核协议事件，解析后触发 `OnTextPacket` 用户事件，插件直接监听即可：

```python
def on_text(args):
    data = args[0]                    # dict
    if data['type'] == 1:             # CHAT
        source = data['source_name']  # str（Unicode）
        msg = data['message']         # str（Unicode）
        print(f'<{source}> {msg}')    # Py3 f-string，中文直接打印

engine.register(on_text, "OnTextPacket")
```

> Py3 下 `source_name` / `message` 已是 `str`，**不要**再调用 `.decode('utf-8')`（str 没有该方法）。

### 处理其他数据包

用 `engine.register_protocol_event(packet_id, cb, False)`：

```python
import pkt

def on_move_player(data):
    # data 是原始二进制（packet_id 已去除），bytes 类型
    runtime_id, off = pkt.read_varint(data, 0)
    x, y, z, off = pkt.unpack("fff", data, off)
    print(f'entity {runtime_id} moved to {x:.1f}, {y:.1f}, {z:.1f}')

# 第三个参数 False = 用户协议事件，重载时自动清除
engine.register_protocol_event(0x13, on_move_player, False)
```

---

## 8. 线程与资源管理

### 线程清理（必须）

如果插件开了后台线程，**必须**实现 `on_uninit()` 函数，否则重载时会崩溃。

**推荐用 `threading` 模块（Py3 标准库）：**

```python
import threading
import time

_running = False

def _worker():
    while _running:
        time.sleep(1)   # 每秒执行一次
        # 业务逻辑

def on_uninit():
    global _running
    _running = False

_running = True
threading.Thread(target=_worker, daemon=True).start()
```

**如果坚持用底层 `_thread`：**

```python
import _thread          # Py3 中 thread 改名 _thread

def _worker():
    while _running:
        pass

_thread.start_new_thread(_worker, ())
```

**同时建议监听 ModEventUnInit：**

```python
def _on_uninit(args):
    on_uninit()

engine.register(_on_uninit, "ModEventUnInit")
```

`on_uninit()` 的调用时机：

- **单插件重载**：`init.py` 在注销事件前主动调用
- **全量重载**：通过 `ModEventUnInit` 事件通知

### 文件资源

- 不要长期持有文件句柄，重载后句柄会失效
- 配置文件建议放在插件目录外（如 `config/`），避免触发热重载
- 临时文件用完即删

---

## 9. MCP 打包发布（遗留）

> **⚠️ 遗留说明**：MCP 模式（`vanilla.mcp` 加密打包）是 Python 2.7 时代的机制。迁移到 Python 3.12 后，**MCP 打包的字节码（Py2 marshal）无法在 Py3 加载**，因此**发布请使用 source 模式**（直接分发 `scripts/` 目录下的 `.py` 源码）。
>
> 代码中的 MCP 相关函数（`fop.new_mcp` / `fop.reload_mcp` / `redirect.mcs` 等）仍然保留，但仅用于兼容旧逻辑，不建议在新插件中使用。

### 旧的打包命令（仅 Py2 时代有效）

```
Program.exe mcp_compile <源目录> <输出文件> [选项]
```

---

## 10. 代码模板

### 最简插件

```python
# scripts/my_plugin/my_plugin/__init__.py

def on_start(args):
    print('MyPlugin loaded!')
    engine.message('', 'Hello, world!')

engine.register(on_start, "ModEventStartUp")
```

### 完整插件结构

```python
import threading
import time

# ── 状态 ──
_running = False

# ── 事件处理 ──
def on_text(args):
    data = args[0]
    print(f"Chat: {data['message']}")      # message 已是 str

def on_command_output(args):
    result, guid = args
    print(f'Command result: {result}')

def on_disconnect(args):
    print('Disconnected from server')
    global _running
    _running = False

# ── 后台线程 ──
def _worker():
    while _running:
        time.sleep(1)   # 每秒执行一次

# ── 生命周期 ──
def on_uninit():
    global _running
    _running = False

def _on_uninit(args):
    on_uninit()

# ── 初始化 ──
engine.register(on_text, "OnTextPacket")
engine.register(on_command_output, "on_command_output")
engine.register(on_disconnect, "on_disconnect")
engine.register(_on_uninit, "ModEventUnInit")

_running = True
threading.Thread(target=_worker, daemon=True).start()
```

### RPC 调用

```python
import umsgpack
engine.send(umsgpack.packb(['ModEventC2S', [None], None]))
```

> `umsgpack` 由框架提供（`source/lib/umsgpack.py`），Py3 下可直接导入。

### 发送指令

```python
engine.command("/say hello")
engine.command("/tp @s 0 100 0")
engine.settingscommand("/gamemode 1 @s")       # 需要管理员权限时优先用
```

---

## 11. 常见问题

### Q: 控制台中文乱码？

A: Py3 下 `str` 是 Unicode，网络数据是 UTF-8，插件内**直接使用即可**。Windows 控制台默认 GBK，`print` 中文可能显示乱码，可用以下方式让控制台正确显示：

```python
import sys
sys.stdout.reconfigure(encoding='utf-8')     # 或 'gbk'
```

（仅影响控制台显示，不影响实际发送/接收的中文。）

### Q: 重载后事件会不会重复注册？

A: 不会。

- **单插件重载**：先精准注销该插件所有事件再重新执行
- **全量重载**：`cleanup_user()` 清空所有用户事件

### Q: `engine.trigger` 传 dict 但回调收到 list？

A: C++ 层会把非 list/tuple 参数包成 `[arg]`。传 dict 请用 `[dict]` 包一层：

```python
engine.trigger("MyEvent", [my_dict])        # 正确
engine.trigger("MyEvent", my_dict)          # 错误！回调会收到 [[my_dict]]
```

回调：

```python
def handler(args):
    data = args[0]      # 收到 my_dict
```

### Q: 怎么发游戏指令？

```python
engine.command("/say hello")
engine.command("/tp @s 0 100 0")
engine.settingscommand("/gamemode creative @s")   # 需要管理员权限
```

### Q: `pkt` 和 `struct` 有什么区别？

A: `pkt` 支持 varint、varint 前缀字符串等 MC 协议特有类型，`struct` 只支持固定长度。解析数据包用 `pkt` 更方便。

### Q: 线程没停导致重载崩溃？

A: 必须实现 `on_uninit()` 停止所有线程。旧线程跑旧代码引用已卸载的模块对象会崩溃。

### Q: 从 Python 2.7 迁移到 3.12，我的旧插件要改什么？

A: 主要改这三类：

1. **语法**：`print 'x'` → `print('x')`；`reload(x)` → `importlib.reload(x)`；`import thread` → `import threading`（或 `_thread`）。
2. **编码**：删除所有 `.decode('utf-8').encode('gbk')`——Py3 下字符串已是 Unicode，直接使用。
3. **pkt 返回值**：`pkt.read_string` / `unpack 's'` 返回 `str`（非 bytes），不要对它再 `.decode()`。

另外，`source/lib/` 里的旧 Py2 标准库副本（`urllib2`、`StringIO` 等）已不再需要，直接用 Py3 标准库（`urllib.request`、`io` 等）。
