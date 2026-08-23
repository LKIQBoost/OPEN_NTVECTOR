#include "PythonApi.h"

#include "Core/ClientInstance.h"
#include "Core/StartupParams.h"
#include "NBT/NbtIo.h"
#include "NbtPython.h"
#include "Packets/BlockDataStore.h"
#include "Packets/EntityStateStore.h"
#include "Packets/SubChunkClient.h"
#include "ErrorPython.h"
#include <pybind11/embed.h>
#include <pybind11/stl.h>
#include <array>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>

namespace py = pybind11;

extern ClientInstance* g_client_instance;

namespace {

struct AsyncSubchunkState {
    std::mutex mutex;
    std::condition_variable condition;
    bool done = false;
    bool cancelled = false;
    bool started = false;
    std::vector<SubChunkClient::BlockData> blocks;
    std::thread worker;
    ~AsyncSubchunkState() { if (worker.joinable()) worker.join(); }
};

class AsyncSubchunkRequest {
public:
    explicit AsyncSubchunkRequest(std::shared_ptr<AsyncSubchunkState> state)
        : state_(std::move(state)) {}

    bool done() const {
        std::lock_guard<std::mutex> lock(state_->mutex);
        return state_->done;
    }

    bool cancelled() const {
        std::lock_guard<std::mutex> lock(state_->mutex);
        return state_->cancelled;
    }

    bool cancel() {
        std::lock_guard<std::mutex> lock(state_->mutex);
        if (state_->started || state_->done) return false;
        state_->cancelled = true;
        state_->done = true;
        state_->condition.notify_all();
        return true;
    }

    py::list result(int timeoutMs) {
        std::unique_lock<std::mutex> lock(state_->mutex);
        if (!state_->done) {
            if (timeoutMs < 0) {
                state_->condition.wait(lock, [&] { return state_->done; });
            } else if (!state_->condition.wait_for(lock, std::chrono::milliseconds(timeoutMs),
                                                   [&] { return state_->done; })) {
                SetApiError(ApiTimeoutError, "asynchronous subchunk request timed out");
                throw py::error_already_set();
            }
        }
        if (state_->cancelled) throw py::value_error("asynchronous subchunk request was cancelled");
        if (state_->worker.joinable()) {
            std::thread worker = std::move(state_->worker);
            lock.unlock();
            worker.join();
            lock.lock();
        }
        py::list result;
        for (const auto& block : state_->blocks) {
            py::dict value;
            value["x"] = block.x;
            value["y"] = block.y;
            value["z"] = block.z;
            value["name"] = block.name;
            value["states"] = block.states_json;
            result.append(std::move(value));
        }
        return result;
    }

private:
    std::shared_ptr<AsyncSubchunkState> state_;
};

py::dict vec3(const Vec3& value)
{
    py::dict result;
    result["x"] = value.x;
    result["y"] = value.y;
    result["z"] = value.z;
    return result;
}

py::dict slot(const SlotItem& value)
{
    py::dict result;
    result["network_id"] = value.NetworkID;
    result["count"] = value.Count;
    result["metadata"] = value.Metadata;
    result["block_runtime_id"] = value.BlockRuntimeID;
    result["stack_id"] = value.HasStackID ? py::cast(value.StackID) : py::none();
    result["extra"] = py::bytes(reinterpret_cast<const char*>(value.Extra.data()), value.Extra.size());
    return result;
}

py::object getLocalPlayer()
{
    if (!g_client_instance || !g_client_instance->getInstance()) return py::none();
    LocalPlayer* player = g_client_instance->getLocalPlayer();
    if (!player) return py::none();

    const LocalPlayerSnapshot state = player->GetSnapshot();
    if (!state.available) return py::none();

    py::dict result;
    result["entity_runtime_id"] = state.entityRuntimeID;
    if (state.positionAvailable) result["position"] = vec3(state.position);
    else result["position"] = py::none();
    if (state.rotationAvailable) {
        py::dict rotation;
        rotation["pitch"] = state.headRotation.x;
        rotation["yaw"] = state.headRotation.y;
        rotation["head_yaw"] = state.headYaw;
        result["rotation"] = std::move(rotation);
    } else {
        result["rotation"] = py::none();
    }
    return std::move(result);
}

py::object getServer()
{
    if (!g_client_instance || !g_client_instance->getInstance()) return py::none();
    LocalPlayer* player = g_client_instance->getLocalPlayer();
    if (!player || !player->GetSnapshot().available) return py::none();
    py::dict result;
    result["ip"] = Params::ServerIP;
    result["port"] = Params::ServerPort;
    result["server_id"] = Params::NeteaseServerID;
    return std::move(result);
}

py::object getBlockActor(int x, int y, int z)
{
    std::vector<uint8_t> raw;
    std::map<std::string, std::string> fields;
    if (!BlockDataStore::GetBlockActor(BlockPos{ x, y, z }, raw, fields)) return py::none();

    py::dict result;
    result["position"] = vec3(Vec3{ static_cast<float>(x), static_cast<float>(y), static_cast<float>(z) });
    result["raw_nbt"] = py::bytes(reinterpret_cast<const char*>(raw.data()), raw.size());

    nbt::ParseOptions options;
    options.encoding = nbt::Encoding::NetworkLittleEndian;
    nbt::ParseResult parsed = nbt::Parse(raw, options);
    if (parsed) {
        PyObject* tag = NbtTagToPythonObject(parsed.root);
        if (!tag) throw py::error_already_set();
        result["nbt"] = py::reinterpret_steal<py::object>(tag);
        result["root_name"] = parsed.rootName;
        result["nbt_error"] = py::none();
    } else {
        result["nbt"] = py::none();
        result["root_name"] = py::none();
        result["nbt_error"] = parsed.error.message + " at byte " + std::to_string(parsed.error.offset);
    }
    return std::move(result);
}

py::object getContainer(int x, int y, int z)
{
    ContainerOpenInfo info;
    std::vector<SlotItem> slots;
    bool hasContent = false;
    if (!BlockDataStore::GetContainerAt(BlockPos{ x, y, z }, info, slots, &hasContent)) return py::none();

    py::dict result;
    result["window_id"] = info.WindowID;
    result["window_type"] = info.WindowType;
    result["position"] = vec3(Vec3{ static_cast<float>(info.Position.x), static_cast<float>(info.Position.y),
                                      static_cast<float>(info.Position.z) });
    result["entity_unique_id"] = info.EntityUniqueID;
    if (hasContent) {
        py::list values;
        for (const SlotItem& value : slots) values.append(slot(value));
        result["slots"] = std::move(values);
    } else {
        result["slots"] = py::none();
    }
    return std::move(result);
}

py::list getPlayers()
{
    py::list result;
    for (const auto& entry : EntityStateStore::GetPlayers()) {
        py::dict value;
        value["uuid"] = py::bytes(reinterpret_cast<const char*>(entry.UUID.data()), entry.UUID.size());
        value["entity_unique_id"] = entry.EntityUniqueID;
        value["username"] = entry.Username;
        value["xuid"] = entry.XUID;
        value["platform_chat_id"] = entry.PlatformChatID;
        value["build_platform"] = entry.BuildPlatform;
        value["teacher"] = entry.Teacher;
        value["host"] = entry.Host;
        value["subclient"] = entry.SubClient;
        value["player_color"] = entry.PlayerColor;
        result.append(std::move(value));
    }
    return result;
}

py::object getWorld()
{
    const WorldStateSnapshot state = EntityStateStore::GetWorld();
    if (!state.available) return py::none();
    py::dict result;
    result["player_entity_id"] = state.playerEntityID;
    result["entity_runtime_id"] = state.entityRuntimeID;
    result["gamemode"] = state.gamemode;
    result["position"] = vec3(state.position);
    return result;
}

py::object getInventory()
{
    std::vector<SlotItem> values;
    if (!BlockDataStore::GetInventory(values)) return py::none();
    py::dict result;
    py::list slots;
    for (const auto& value : values) slots.append(slot(value));
    result["window_id"] = 0;
    result["slots"] = std::move(slots);
    return result;
}

std::shared_ptr<AsyncSubchunkRequest> requestSubchunksAsync(
    int ox, int oy, int oz, const std::vector<std::array<int8_t, 3>>& offsets, int timeoutMs)
{
    if (offsets.empty()) throw py::value_error("offsets must not be empty");
    if (timeoutMs < 0) throw py::value_error("timeout_ms must not be negative");

    auto state = std::make_shared<AsyncSubchunkState>();
    state->worker = std::thread([state, ox, oy, oz, offsets, timeoutMs] {
        {
            std::lock_guard<std::mutex> lock(state->mutex);
            if (state->cancelled) return;
            state->started = true;
        }
        std::vector<SubChunkClient::BlockData> blocks;
        const bool ok = SubChunkClient::RequestBlocks(ox, oy, oz, offsets, blocks, timeoutMs);
        {
            std::lock_guard<std::mutex> lock(state->mutex);
            if (ok) state->blocks = std::move(blocks);
            state->done = true;
        }
        state->condition.notify_all();
    });
    return std::make_shared<AsyncSubchunkRequest>(std::move(state));
}

} // namespace

PYBIND11_MODULE(game_state, module)
{
    module.doc() = "Queries for server-confirmed bot state; unavailable information is returned as None";
    module.def("get_local_player", &getLocalPlayer);
    module.def("has_local_player", [] { return !getLocalPlayer().is_none(); });
    module.def("get_server", &getServer);
    module.def("get_block_actor", &getBlockActor, py::arg("x"), py::arg("y"), py::arg("z"));
    module.def("get_container", &getContainer, py::arg("x"), py::arg("y"), py::arg("z"));
    module.def("get_players", &getPlayers);
    module.def("get_world", &getWorld);
    module.def("get_inventory", &getInventory);
    py::class_<AsyncSubchunkRequest, std::shared_ptr<AsyncSubchunkRequest>>(
        module, "AsyncSubchunkRequest")
        .def("done", &AsyncSubchunkRequest::done)
        .def("cancel", &AsyncSubchunkRequest::cancel)
        .def("cancelled", &AsyncSubchunkRequest::cancelled)
        .def("result", &AsyncSubchunkRequest::result, py::arg("timeout_ms") = -1);
    module.def("request_subchunks_async", &requestSubchunksAsync,
               py::arg("origin_x"), py::arg("origin_y"), py::arg("origin_z"),
               py::arg("offsets"), py::arg("timeout_ms") = 6000);
}
