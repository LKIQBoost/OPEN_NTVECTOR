#include "StructuredProtocol.h"

#include "NbtPython.h"
#include "Packets/BlockActorData.h"
#include "Packets/ContainerOpen.h"
#include "Packets/InventoryContent.h"
#include "Packets/MovePlayer.h"
#include "Packets/PlayStatus.h"
#include "Packets/Text.h"
#include "Packets/CommandOutput.h"
#include "Packets/PlayerList.h"
#include "Packets/PacketBase.h"

namespace {

void setItem(PyObject* dict, const char* key, PyObject* value)
{
    if (!value) return;
    PyDict_SetItemString(dict, key, value);
    Py_DECREF(value);
}

PyObject* position(int x, int y, int z)
{
    PyObject* result = PyDict_New();
    setItem(result, "x", PyLong_FromLong(x));
    setItem(result, "y", PyLong_FromLong(y));
    setItem(result, "z", PyLong_FromLong(z));
    return result;
}

PyObject* position(const Vec3& value)
{
    PyObject* result = PyDict_New();
    setItem(result, "x", PyFloat_FromDouble(value.x));
    setItem(result, "y", PyFloat_FromDouble(value.y));
    setItem(result, "z", PyFloat_FromDouble(value.z));
    return result;
}

PyObject* slot(const SlotItem& value)
{
    PyObject* result = PyDict_New();
    setItem(result, "network_id", PyLong_FromLong(value.NetworkID));
    setItem(result, "count", PyLong_FromUnsignedLong(value.Count));
    setItem(result, "metadata", PyLong_FromUnsignedLong(value.Metadata));
    setItem(result, "block_runtime_id", PyLong_FromLong(value.BlockRuntimeID));
    if (value.HasStackID) setItem(result, "stack_id", PyLong_FromLong(value.StackID));
    else { Py_INCREF(Py_None); setItem(result, "stack_id", Py_None); }
    setItem(result, "extra", PyBytes_FromStringAndSize(
        reinterpret_cast<const char*>(value.Extra.data()), value.Extra.size()));
    return result;
}

PyObject* decodeKnownPacket(uint32_t packetId, const std::string& payload, std::string& error)
{
    const bool supported = packetId == IDContainerOpen || packetId == IDInventoryContent ||
        packetId == IDBlockActorData || packetId == IDMovePlayer || packetId == IDPlayStatus ||
        packetId == IDText || packetId == IDCommandOutput || packetId == IDPlayerList;
    if (!supported) return nullptr;

    const std::vector<unsigned char> bytes(payload.begin(), payload.end());
    if (bytes.empty()) {
        error = "packet payload is empty";
        return nullptr;
    }

    if (packetId == IDContainerOpen) {
        ContainerOpen packet;
        packet.Deserializ(bytes);
        PyObject* result = PyDict_New();
        setItem(result, "window_id", PyLong_FromLong(packet.WindowID));
        setItem(result, "window_type", PyLong_FromLong(packet.WindowType));
        setItem(result, "position", position(packet.Position.x, packet.Position.y, packet.Position.z));
        setItem(result, "entity_unique_id", PyLong_FromLongLong(packet.EntityUniqueID));
        return result;
    }

    if (packetId == IDInventoryContent) {
        InventoryContent packet;
        packet.Deserializ(bytes);
        PyObject* result = PyDict_New();
        setItem(result, "window_id", PyLong_FromLong(packet.WindowID));
        PyObject* slots = PyList_New(packet.Slots.size());
        for (size_t i = 0; i < packet.Slots.size(); ++i) PyList_SetItem(slots, i, slot(packet.Slots[i]));
        setItem(result, "slots", slots);
        setItem(result, "container_id", PyLong_FromUnsignedLong(packet.ContainerID));
        if (packet.HasDynamicContainerID) {
            setItem(result, "dynamic_container_id", PyLong_FromUnsignedLong(packet.DynamicContainerID));
        } else {
            Py_INCREF(Py_None);
            setItem(result, "dynamic_container_id", Py_None);
        }
        return result;
    }

    if (packetId == IDBlockActorData) {
        BlockActorData packet;
        packet.Deserializ(bytes);
        PyObject* result = PyDict_New();
        setItem(result, "position", position(packet.Position.x, packet.Position.y, packet.Position.z));
        setItem(result, "raw_nbt", PyBytes_FromStringAndSize(
            reinterpret_cast<const char*>(packet.RawNBT.data()), packet.RawNBT.size()));
        if (packet.NBTParsed) {
            setItem(result, "nbt", NbtTagToPythonObject(packet.NBT));
            setItem(result, "root_name", PyUnicode_FromStringAndSize(packet.NBTRootName.data(), packet.NBTRootName.size()));
        } else {
            error = packet.NBTError;
            Py_INCREF(Py_None);
            setItem(result, "nbt", Py_None);
            Py_INCREF(Py_None);
            setItem(result, "root_name", Py_None);
        }
        return result;
    }

    if (packetId == IDMovePlayer) {
        MovePlayer packet;
        packet.Deserializ(bytes);
        PyObject* result = PyDict_New();
        setItem(result, "entity_runtime_id", PyLong_FromLongLong(packet.EntityRuntimeID));
        setItem(result, "position", position(packet.Position));
        setItem(result, "pitch", PyFloat_FromDouble(packet.Pitch));
        setItem(result, "yaw", PyFloat_FromDouble(packet.Yaw));
        setItem(result, "head_yaw", PyFloat_FromDouble(packet.HeadYaw));
        return result;
    }

    if (packetId == IDPlayStatus) {
        PlayStatus packet;
        packet.Deserializ(bytes);
        PyObject* result = PyDict_New();
        setItem(result, "status", PyLong_FromLong(packet.Status));
        return result;
    }

    if (packetId == IDText) {
        Text packet;
        packet.Deserializ(bytes);
        PyObject* result = PyDict_New();
        setItem(result, "type", PyLong_FromUnsignedLong(packet.type));
        setItem(result, "data", PyUnicode_FromStringAndSize(packet._data.data(), packet._data.size()));
        setItem(result, "data2", PyUnicode_FromStringAndSize(packet._data2.data(), packet._data2.size()));
        setItem(result, "system_message", PyUnicode_FromStringAndSize(packet.SysMsg.data(), packet.SysMsg.size()));
        return result;
    }

    if (packetId == IDCommandOutput) {
        CommandOutput packet;
        packet.Deserializ(bytes);
        PyObject* result = PyDict_New();
        setItem(result, "origin", PyLong_FromUnsignedLong(packet.Origin.Origin));
        setItem(result, "request_id", PyUnicode_FromStringAndSize(packet.Origin.RequestID.data(), packet.Origin.RequestID.size()));
        setItem(result, "output_type", PyLong_FromUnsignedLong(packet.OutputType));
        setItem(result, "success_count", PyLong_FromUnsignedLong(packet.SuccessCount));
        PyObject* messages = PyList_New(packet.OutputMessages.size());
        for (size_t i = 0; i < packet.OutputMessages.size(); ++i) {
            PyObject* message = PyDict_New();
            setItem(message, "success", PyBool_FromLong(packet.OutputMessages[i].Success));
            setItem(message, "message", PyUnicode_FromStringAndSize(
                packet.OutputMessages[i].Message.data(), packet.OutputMessages[i].Message.size()));
            PyList_SetItem(messages, i, message);
        }
        setItem(result, "messages", messages);
        setItem(result, "data_set", PyUnicode_FromStringAndSize(packet.DataSet.data(), packet.DataSet.size()));
        return result;
    }

    if (packetId == IDPlayerList) {
        PlayerListPacket packet;
        packet.Deserializ(bytes);
        PyObject* result = PyDict_New();
        setItem(result, "action", PyLong_FromUnsignedLong(packet.ActionType));
        PyObject* entries = PyList_New(packet.Entries.size());
        for (size_t i = 0; i < packet.Entries.size(); ++i) {
            const auto& entry = packet.Entries[i];
            PyObject* value = PyDict_New();
            setItem(value, "uuid", PyBytes_FromStringAndSize(
                reinterpret_cast<const char*>(entry.UUID.data()), entry.UUID.size()));
            setItem(value, "entity_unique_id", PyLong_FromLongLong(entry.EntityUniqueID));
            setItem(value, "username", PyUnicode_FromStringAndSize(entry.Username.data(), entry.Username.size()));
            setItem(value, "xuid", PyUnicode_FromStringAndSize(entry.XUID.data(), entry.XUID.size()));
            setItem(value, "platform_chat_id", PyUnicode_FromStringAndSize(
                entry.PlatformChatID.data(), entry.PlatformChatID.size()));
            PyList_SetItem(entries, i, value);
        }
        setItem(result, "entries", entries);
        return result;
    }

    return nullptr;
}

} // namespace

const char* GetProtocolPacketName(uint32_t packetId) noexcept
{
    switch (packetId) {
    case IDLogin: return "Login";
    case IDPlayStatus: return "PlayStatus";
    case IDDisconnect: return "Disconnect";
    case IDStartGame: return "StartGame";
    case IDMovePlayer: return "MovePlayer";
    case IDContainerOpen: return "ContainerOpen";
    case IDInventoryContent: return "InventoryContent";
    case IDBlockActorData: return "BlockActorData";
    case IDCommandRequest: return "CommandRequest";
    case IDCommandOutput: return "CommandOutput";
    case IDPlayerList: return "PlayerList";
    case IDPlayerAuthInput: return "PlayerAuthInput";
    case IDSubChunk: return "SubChunk";
    case IDPyRpc: return "PyRpc";
    default: return "Unknown";
    }
}

PyObject* BuildStructuredProtocolEvent(uint32_t packetId, const std::string& payload)
{
    PyObject* event = PyDict_New();
    if (!event) return nullptr;

    setItem(event, "packet_id", PyLong_FromUnsignedLong(packetId));
    setItem(event, "packet_name", PyUnicode_FromString(GetProtocolPacketName(packetId)));
    setItem(event, "payload", PyBytes_FromStringAndSize(payload.data(), payload.size()));
    setItem(event, "size", PyLong_FromSize_t(payload.size()));

    std::string error;
    PyObject* data = DecodeStructuredProtocolData(packetId, payload, error);
    setItem(event, "parsed", PyBool_FromLong(data != nullptr && error.empty()));
    if (data) setItem(event, "data", data);
    else { Py_INCREF(Py_None); setItem(event, "data", Py_None); }

    if (error.empty()) { Py_INCREF(Py_None); setItem(event, "error", Py_None); }
    else setItem(event, "error", PyUnicode_FromStringAndSize(error.data(), error.size()));
    return event;
}

PyObject* DecodeStructuredProtocolData(uint32_t packetId, const std::string& payload,
                                        std::string& error)
{
    try {
        return decodeKnownPacket(packetId, payload, error);
    } catch (const std::exception& exception) {
        error = exception.what();
        return nullptr;
    } catch (...) {
        error = "unknown packet decode error";
        return nullptr;
    }
}
