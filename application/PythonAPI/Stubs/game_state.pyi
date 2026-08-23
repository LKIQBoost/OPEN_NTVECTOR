from typing import TypedDict
from nbt import Tag

class Vec3(TypedDict):
    x: float
    y: float
    z: float

class Rotation(TypedDict):
    pitch: float
    yaw: float
    head_yaw: float

class LocalPlayerState(TypedDict):
    entity_runtime_id: int
    position: Vec3 | None
    rotation: Rotation | None

class PlayerState(TypedDict):
    uuid: bytes
    entity_unique_id: int
    username: str
    xuid: str
    platform_chat_id: str
    build_platform: int
    teacher: bool
    host: bool
    subclient: bool
    player_color: int

class WorldState(TypedDict):
    player_entity_id: int
    entity_runtime_id: int
    gamemode: int
    position: Vec3

class AsyncSubchunkRequest:
    def done(self) -> bool: ...
    def cancel(self) -> bool: ...
    def cancelled(self) -> bool: ...
    def result(self, timeout_ms: int = -1) -> list[dict[str, object]]: ...

def get_local_player() -> LocalPlayerState | None: ...
def has_local_player() -> bool: ...
def get_server() -> dict[str, object] | None: ...
def get_block_actor(x: int, y: int, z: int) -> dict[str, object] | None: ...
def get_container(x: int, y: int, z: int) -> dict[str, object] | None: ...
def get_players() -> list[PlayerState]: ...
def get_world() -> WorldState | None: ...
def get_inventory() -> dict[str, object] | None: ...
def request_subchunks_async(origin_x: int, origin_y: int, origin_z: int,
                            offsets: list[tuple[int, int, int]],
                            timeout_ms: int = ...) -> AsyncSubchunkRequest: ...
