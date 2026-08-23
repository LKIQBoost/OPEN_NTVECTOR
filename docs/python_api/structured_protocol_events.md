# 结构化协议事件

## 注册事件

```python
import engine

def on_packet(event):
    print(event["packet_id"], event["packet_name"], event["size"])
    if event["parsed"]:
        print(event["data"])
    elif event["error"] is not None:
        print("parse failed:", event["error"])

engine.register_structured_protocol_event(56, on_packet)
```

第三个参数 `is_kernel` 默认为 `False`。内核事件只应由框架自身注册。

## 事件结构

每次回调接收一个字典：

| 字段 | 类型 | 含义 |
| --- | --- | --- |
| `packet_id` | `int` | Bedrock 协议包 ID |
| `packet_name` | `str` | 已知包名，未知时为 `Unknown` |
| `payload` | `bytes` | 不含包 ID 的原始负载 |
| `size` | `int` | 原始负载长度 |
| `parsed` | `bool` | 是否已可靠解析为 `data` |
| `data` | `dict | None` | 已解析字段，不支持解析时为 `None` |
| `error` | `str | None` | 解析错误，没有错误时为 `None` |

当前提供字段解析的包包括 `MovePlayer`、`ContainerOpen`、`InventoryContent`、
`BlockActorData`、`PlayStatus`、`Text`、`CommandOutput` 和 `PlayerList`。其他包
仍会提供完整事件信封和原始负载，但 `parsed` 为 `False`。解析异常会保留在
`error` 字段中，不会中断其他协议回调。

原有 `register_protocol_event` 保持不变，回调仍只接收 `bytes`。

## NBT 示例

```python
def on_block_actor(event):
    if not event["parsed"]:
        return
    tag = event["data"]["nbt"]
    command = tag.Get("Command")
    if command is not None:
        print(command.GetValue())

engine.register_structured_protocol_event(56, on_block_actor)
```
