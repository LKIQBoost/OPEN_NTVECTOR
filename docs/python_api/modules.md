# Python 原生接口参考

本文档对应 `application/PythonAPI/Stubs` 中的类型声明和运行时注册模块。
函数名、参数顺序和返回类型以 C++ 导出表为准。

## 核心模块

### `api_errors`

提供 `BotApiError`、`NbtError`、`PacketError`、`ConnectionError`、
`CryptoError` 和 `ClientError`。所有专用异常都继承 `BotApiError`。

### `nbt`

提供 `TagType`、`Tag`、`Document`，以及 `loads`、`load_tag`、`dumps`、
`dump_document`、`flatten`。支持 `big_endian`、`little_endian` 和
`network_little_endian`。详细示例见 [`native_modules.md`](native_modules.md)。

### `packets`

提供 `CommandOrigin` 和 `CommandRequest`。`CommandRequest` 支持命令、来源、
UUID、请求 ID、玩家 ID、内部来源、命令版本和尾部字段的 getter/setter，另有
`GetPacketID`、`Serialize` 和 `Send`。

`Packet(packet_id, payload)` 表示任意协议包的原始对象。`encode`、`decode` 和
`decode_bytes` 提供统一的包 ID/负载编解码；已支持结构化解析的包可以通过
`Packet.GetData()` 获取字段字典。

### `game_state`

| 函数 | 返回值 |
| --- | --- |
| `get_local_player()` | `LocalPlayerState | None` |
| `has_local_player()` | `bool` |
| `get_server()` | 服务器信息字典或 `None` |
| `get_block_actor(x, y, z)` | 方块实体/NBT 字典或 `None` |
| `get_container(x, y, z)` | 容器和物品槽字典或 `None` |
| `get_players()` | 当前玩家列表 |
| `get_world()` | StartGame 确认的世界/玩家基础状态 |
| `get_inventory()` | 窗口 0 的玩家背包或 `None` |
| `request_subchunks_async(...)` | 返回可等待、可取消的异步请求句柄 |

### `engine`

| 类别 | 函数 |
| --- | --- |
| 事件 | `register`、`register_kernel_event`、`unregister`、`is_registered`、`trigger` |
| 事件查询 | `get_event_handler_count`、`get_events_info`、`get_handler_count` |
| 清理 | `cleanup_all`、`cleanup_user`、`cleanup_kernel` |
| 协议事件 | `register_protocol_event`、`register_structured_protocol_event`、`clear_all_protocol_event`、`clear_all_kernel_protocol_event`、`clear_all_user_protocol_event` |
| 指令和数据 | `message`、`command`、`command_guid`、`send`、`rpc`、`settingscommand`、`command_update`、`openContainer` |
| 状态 | `get_server_ip`、`get_server_port`、`get_server_sid`、`getparams`、`get_entity_runtime_id` |
| 移动视角 | `move`、`add_pot_x`、`add_pot_y`、`add_pot_z`、`get_pot_x`、`get_pot_y`、`get_pot_z`、`add_head_x`、`add_head_y`、`get_head_x`、`get_head_y` |
| 其他 | `get_subchunk_blocks`、`get_auth_input`、`disabled_auth_input`、`enable_auth_input`、`respawn`、`get_mcp_load_config` |

坐标和实体状态在服务器尚未确认时返回 `None`。`openContainer` 发起异步请求，
返回的缓存数据可能为空。

`get_players()` 返回最近收到的 `PlayerList` 玩家快照；`get_world()` 返回最近
收到的 `StartGame` 基础世界快照；`get_inventory()` 查询窗口 0 的背包内容。
这些接口只反映服务端已确认并缓存的数据，不会用空列表伪装“尚未收到”。

## 网络和客户端模块

### `_raknet`

`get_raknet()` 创建 `RakNet` 对象。对象提供 `Startup`、`Connect`、`Send`、
`Receive`、`ReceivePacket`、`Disconnect`、`Shutdown`、`GetInstanceId`、
`GetConnectionState` 和 `GetReceiveQueueSize`。`ReceivePacket` 返回包含
`message_id`、`payload`、`address` 的字典或 `None`。

### `_websocket`

`WebSocket(ip, port, path)` 提供 `connect`、`send`、`close` 和只读的
`event_id`；模块级 `get_websocket` 和 `delete` 保留旧版工厂接口。连接失败
抛出 `ConnectionError`。

### `_client` 与 `client_instance`

两者都是兼容旧插件的客户端启动入口。`_client` 使用 capsule 实例，提供
`get_client`、`startUp`、`disconnect`、`delete`；`client_instance` 提供直接的
`startUp` 和 `disconnect`。

### `tan_lobby_game_clicpp_wrapper`

`create(disout=False)` 返回 `TanLobbyGameCtx`，其 `startUp` 建立 NetherNet
大厅连接。

## 工具和兼容模块

| 模块 | 导出 |
| --- | --- |
| `pkt` | `read_*`、`write_*`、`pack`、`unpack`、`remaining` 协议二进制工具 |
| `utility` | `decrypt_with_tail`、`encrypt_with_tail`、`get_encrypt_token` |
| `easy_utils` | `ComputeDynamicToken`、`HttpEncrypt`、`HttpDecrypt`、`encrypt`、`Request` |
| `aes` | `ecb128encrypt`、`ecb128decrypt` |
| `_chacha` | `get_chacha`、`process`、`delete` |
| `rotor` | `newrotor` 和 rotor 对象的加解密方法 |
| `setting` | `get_token`、`get_playerid`、`get_engine_version`、`get_patch_version`、`get_uid`、`get_name` |
| `mod_log` | `log(level, message)` |
| `fop` | `find_file`、`get_file`、`new_module`、`new_mcp`、`reload_mcp` |

`fop` 和 `_client` 是遗留兼容接口；新插件优先使用 `client_instance`、`engine`、
`packets` 和 `game_state`。

`request_subchunks_async` 返回的请求句柄默认允许 `result()` 无限等待；传入
非负毫秒数即可限制等待时间，超时抛出 `api_errors.TimeoutError`。`cancel()` 只
取消尚未开始的请求，已经发送到服务器的请求会继续由底层协议完成。
