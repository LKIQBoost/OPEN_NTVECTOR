#include "PythonApi.h"

#include "Core/ClientInstance.h"
#include "ErrorPython.h"
#include "Packets/CommandRequest.h"
#include "StructuredProtocol.h"
#include "Packets/PacketBase.h"
#include <pybind11/embed.h>
#include <pybind11/stl.h>

namespace py = pybind11;

extern ClientInstance* g_client_instance;

namespace {

class GenericPacket {
public:
    GenericPacket(uint32_t packetId, std::vector<unsigned char> payload)
        : packetId_(packetId), payload_(std::move(payload)) {}

    uint32_t GetPacketID() const noexcept { return packetId_; }
    std::string GetPacketName() const { return GetProtocolPacketName(packetId_); }
    py::bytes GetPayload() const {
        return py::bytes(reinterpret_cast<const char*>(payload_.data()), payload_.size());
    }
    py::object GetData() const {
        std::string error;
        PyObject* value = DecodeStructuredProtocolData(packetId_,
            std::string(reinterpret_cast<const char*>(payload_.data()), payload_.size()), error);
        if (!value) return py::none();
        return py::reinterpret_steal<py::object>(value);
    }
    py::bytes Serialize() const {
        std::vector<unsigned char> output;
        uint32_t value = packetId_;
        do {
            unsigned char byte = static_cast<unsigned char>(value & 0x7f);
            value >>= 7;
            if (value) byte |= 0x80;
            output.push_back(byte);
        } while (value);
        output.insert(output.end(), payload_.begin(), payload_.end());
        return py::bytes(reinterpret_cast<const char*>(output.data()), output.size());
    }

private:
    uint32_t packetId_;
    std::vector<unsigned char> payload_;
};

GenericPacket decodePacket(uint32_t packetId, const py::bytes& payload)
{
    const std::string value = payload;
    return GenericPacket(packetId,
        std::vector<unsigned char>(value.begin(), value.end()));
}

GenericPacket decodePacketBytes(const py::bytes& data)
{
    const std::string value = data;
    uint32_t packetId = 0;
    unsigned int shift = 0;
    size_t offset = 0;
    while (offset < value.size() && shift < 35) {
        const unsigned char byte = static_cast<unsigned char>(value[offset++]);
        packetId |= static_cast<uint32_t>(byte & 0x7f) << shift;
        if (!(byte & 0x80)) {
            return GenericPacket(packetId,
                std::vector<unsigned char>(value.begin() + offset, value.end()));
        }
        shift += 7;
    }
    SetApiError(ApiPacketError, "invalid packet id varint");
    throw py::error_already_set();
}

py::bytes encodePacket(uint32_t packetId, const py::bytes& payload)
{
    return decodePacket(packetId, payload).Serialize();
}

void setRandomUuid(CommandRequest& packet, const py::bytes& value)
{
    const std::string bytes = value;
    if (bytes.size() != 16) {
        SetApiError(ApiPacketError, "CommandRequest UUID must contain exactly 16 bytes");
        throw py::error_already_set();
    }
    packet.SetRandomUUID(bytes);
}

py::bytes getRandomUuid(const CommandRequest& packet)
{
    const std::string& value = packet.GetRandomUUID();
    return py::bytes(value.data(), value.size());
}

py::bytes serialize(CommandRequest& packet)
{
    const std::vector<unsigned char> data = packet.Serializ();
    return py::bytes(reinterpret_cast<const char*>(data.data()), data.size());
}

int sendCommandRequest(CommandRequest& packet)
{
    if (!g_client_instance || !g_client_instance->getInstance()) {
        SetApiError(ApiConnectionError, "client connection is not available");
        throw py::error_already_set();
    }
    return g_client_instance->getInstance()->WritePacket(packet);
}

} // namespace

PYBIND11_MODULE(packets, module)
{
    module.doc() = "Version-compatible packet object interfaces";
    module.attr("PacketError") = py::reinterpret_borrow<py::object>(ApiPacketError);
    module.attr("ConnectionError") = py::reinterpret_borrow<py::object>(ApiConnectionError);

    py::class_<GenericPacket>(module, "Packet")
        .def(py::init([](uint32_t packetId, const py::bytes& payload) {
            return decodePacket(packetId, payload);
        }))
        .def("GetPacketID", &GenericPacket::GetPacketID)
        .def("GetPacketName", &GenericPacket::GetPacketName)
        .def("GetPayload", &GenericPacket::GetPayload)
        .def("GetData", &GenericPacket::GetData)
        .def("Serialize", &GenericPacket::Serialize);
    module.def("decode", &decodePacket, py::arg("packet_id"), py::arg("payload"));
    module.def("decode_bytes", &decodePacketBytes, py::arg("data"));
    module.def("encode", &encodePacket, py::arg("packet_id"), py::arg("payload"));

    py::enum_<CommandOriginType>(module, "CommandOrigin")
        .value("Player", CommandOriginType::Player)
        .value("CommandBlock", CommandOriginType::CommandBlock)
        .value("MinecartCommandBlock", CommandOriginType::MinecartCommandBlock)
        .value("DevConsole", CommandOriginType::DevConsole)
        .value("Test", CommandOriginType::Test)
        .value("AutomationPlayer", CommandOriginType::AutomationPlayer)
        .value("ClientAutomation", CommandOriginType::ClientAutomation)
        .value("DedicatedServer", CommandOriginType::DedicatedServer)
        .value("Entity", CommandOriginType::Entity)
        .value("Virtual", CommandOriginType::Virtual)
        .value("GameArgument", CommandOriginType::GameArgument)
        .value("EntityServer", CommandOriginType::EntityServer)
        .value("Precompiled", CommandOriginType::Precompiled)
        .value("GameDirectorEntityServer", CommandOriginType::GameDirectorEntityServer)
        .value("Scripting", CommandOriginType::Scripting)
        .value("ExecuteContext", CommandOriginType::ExecuteContext);

    py::class_<CommandRequest>(module, "CommandRequest")
        .def(py::init<>())
        .def("SetCommand", &CommandRequest::SetCommand)
        .def("GetCommand", &CommandRequest::GetCommand)
        .def("SetCommandOrigin", &CommandRequest::SetCommandOrigin)
        .def("GetCommandOrigin", &CommandRequest::GetCommandOrigin)
        .def("SetRandomUUID", &setRandomUuid)
        .def("GetRandomUUID", &getRandomUuid)
        .def("SetRequestID", &CommandRequest::SetRequestID)
        .def("GetRequestID", &CommandRequest::GetRequestID)
        .def("SetPlayerID", &CommandRequest::SetPlayerID)
        .def("GetPlayerID", &CommandRequest::GetPlayerID)
        .def("SetInternalSource", &CommandRequest::SetInternalSource)
        .def("GetInternalSource", &CommandRequest::GetInternalSource)
        .def("SetCommandVersion", &CommandRequest::SetCommandVersion)
        .def("GetCommandVersion", &CommandRequest::GetCommandVersion)
        .def("SetUnlimit", &CommandRequest::SetUnlimit)
        .def("GetUnlimit", &CommandRequest::GetUnlimit)
        .def("GetPacketID", [](CommandRequest& packet) { return packet.ID(); })
        .def("Serialize", &serialize)
        .def("Send", &sendCommandRequest);
}
