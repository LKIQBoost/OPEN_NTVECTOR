#include <Python.h>

#include <SLikeNet/BitStream.h>
#include <MessageIdentifiers.h>
#include <PacketPriority.h>
#include <RakPeerInterface.h>
#include <RakNetTypes.h>

#include "Logger.h"
#include "ErrorPython.h"
#include "engine_wrapper.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstring>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace {

struct ReceivedMessage {
    uint8_t messageId = 0;
    std::vector<uint8_t> payload;
    std::string address;
};

typedef struct {
    PyObject_HEAD
    SLNet::RakPeerInterface* peer;
    SLNet::SystemAddress* systemAddress;
    SLNet::SocketDescriptor* socketDescriptor;
    std::thread* recvThread;
    std::atomic<bool>* running;
    std::mutex* queueMutex;
    std::condition_variable* queueCondition;
    std::deque<ReceivedMessage>* receiveQueue;
    int instanceId;
    bool started;
} PyRakNet;

PyTypeObject PyRakNet_Type = { PyVarObject_HEAD_INIT(nullptr, 0) };
std::atomic<int> g_nextInstanceId{ 1 };
constexpr size_t MaxQueuedMessages = 4096;

SLNet::SystemAddress getRemoteAddress(PyRakNet* self)
{
    std::lock_guard<std::mutex> lock(*self->queueMutex);
    return *self->systemAddress;
}

void setRemoteAddress(PyRakNet* self, const SLNet::SystemAddress& address)
{
    std::lock_guard<std::mutex> lock(*self->queueMutex);
    *self->systemAddress = address;
}

bool isReceiveThread(const PyRakNet* self)
{
    return self->recvThread && self->recvThread->joinable() &&
        self->recvThread->get_id() == std::this_thread::get_id();
}

void queuePacket(PyRakNet* self, const SLNet::Packet& packet)
{
    ReceivedMessage message;
    const bool hasData = packet.length > 0 && packet.data;
    message.messageId = hasData ? packet.data[0] : 0;
    if (hasData && packet.length > 1) {
        message.payload.assign(packet.data + 1, packet.data + packet.length);
    }
    message.address = packet.systemAddress.ToString(false);

    {
        std::lock_guard<std::mutex> lock(*self->queueMutex);
        if (self->receiveQueue->size() == MaxQueuedMessages) self->receiveQueue->pop_front();
        self->receiveQueue->push_back(std::move(message));
    }
    self->queueCondition->notify_one();
}

void receiveLoop(PyRakNet* self)
{
    Logger::getInstance().log(LOG_RAKNET, "[RakNet] receive thread started, instance_id: " +
        std::to_string(self->instanceId));

    while (self->running->load()) {
        if (!self->peer) break;

        SLNet::Packet* packet = nullptr;
        while (self->running->load() && (packet = self->peer->Receive()) != nullptr) {
            queuePacket(self, *packet);
            const uint8_t messageId = packet->length > 0 ? packet->data[0] : 0;
            const std::string address = packet->systemAddress.ToString(false);

            switch (messageId) {
            case ID_CONNECTION_REQUEST_ACCEPTED: {
                setRemoteAddress(self, packet->systemAddress);
                PythonEventEngine engine;
                engine.trigger("ID_CONNECTION_REQUEST_ACCEPTED", self->instanceId, address);
                break;
            }
            case ID_DISCONNECTION_NOTIFICATION: {
                setRemoteAddress(self, SLNet::UNASSIGNED_SYSTEM_ADDRESS);
                PythonEventEngine engine;
                engine.trigger("ID_DISCONNECTION_NOTIFICATION", self->instanceId);
                break;
            }
            case ID_CONNECTION_LOST: {
                setRemoteAddress(self, SLNet::UNASSIGNED_SYSTEM_ADDRESS);
                PythonEventEngine engine;
                engine.trigger("ID_CONNECTION_LOST", self->instanceId);
                break;
            }
            case ID_CONNECTION_ATTEMPT_FAILED: {
                setRemoteAddress(self, SLNet::UNASSIGNED_SYSTEM_ADDRESS);
                PythonEventEngine engine;
                engine.trigger("ID_CONNECTION_ATTEMPT_FAILED", self->instanceId);
                break;
            }
            case 0xfe: {
                PythonEventEngine engine;
                const std::string body(
                    reinterpret_cast<const char*>(packet->data + 1), packet->length > 0 ? packet->length - 1 : 0);
                engine.trigger("on_raknet_message", self->instanceId, body, address);
                break;
            }
            default:
                break;
            }

            self->peer->DeallocatePacket(packet);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    self->queueCondition->notify_all();
    Logger::getInstance().log(LOG_RAKNET, "[RakNet] receive thread stopped, instance_id: " +
        std::to_string(self->instanceId));
}

void stopReceiveThread(PyRakNet* self)
{
    if (self->running) self->running->store(false);
    if (self->queueCondition) self->queueCondition->notify_all();
    if (self->recvThread) {
        if (self->recvThread->joinable() && self->recvThread->get_id() != std::this_thread::get_id()) {
            self->recvThread->join();
        }
        delete self->recvThread;
        self->recvThread = nullptr;
    }
}

void cleanup(PyRakNet* self, bool destroyPeer)
{
    stopReceiveThread(self);
    if (self->peer && self->started) self->peer->Shutdown(300);
    self->started = false;
    if (self->systemAddress) setRemoteAddress(self, SLNet::UNASSIGNED_SYSTEM_ADDRESS);
    if (destroyPeer && self->peer) {
        SLNet::RakPeerInterface::DestroyInstance(self->peer);
        self->peer = nullptr;
    }
}

PyObject* raknetNew(PyTypeObject* type, PyObject*, PyObject*)
{
    PyRakNet* self = reinterpret_cast<PyRakNet*>(type->tp_alloc(type, 0));
    if (!self) return nullptr;

    self->peer = SLNet::RakPeerInterface::GetInstance();
    self->systemAddress = new SLNet::SystemAddress(SLNet::UNASSIGNED_SYSTEM_ADDRESS);
    self->socketDescriptor = new SLNet::SocketDescriptor();
    self->recvThread = nullptr;
    self->running = new std::atomic<bool>(false);
    self->queueMutex = new std::mutex();
    self->queueCondition = new std::condition_variable();
    self->receiveQueue = new std::deque<ReceivedMessage>();
    self->instanceId = g_nextInstanceId.fetch_add(1);
    self->started = false;
    return reinterpret_cast<PyObject*>(self);
}

void raknetDealloc(PyRakNet* self)
{
    Py_BEGIN_ALLOW_THREADS
    cleanup(self, true);
    Py_END_ALLOW_THREADS
    delete self->running;
    delete self->queueMutex;
    delete self->queueCondition;
    delete self->receiveQueue;
    delete self->systemAddress;
    delete self->socketDescriptor;
    self->running = nullptr;
    self->queueMutex = nullptr;
    self->queueCondition = nullptr;
    self->receiveQueue = nullptr;
    self->systemAddress = nullptr;
    self->socketDescriptor = nullptr;
    Py_TYPE(self)->tp_free(reinterpret_cast<PyObject*>(self));
}

PyObject* raknetGet(PyObject*, PyObject*)
{
    return PyObject_CallNoArgs(reinterpret_cast<PyObject*>(&PyRakNet_Type));
}

PyObject* raknetDelete(PyObject*, PyObject* args)
{
    PyObject* object = nullptr;
    if (!PyArg_ParseTuple(args, "O", &object)) return nullptr;
    if (!PyObject_TypeCheck(object, &PyRakNet_Type)) {
        PyErr_SetString(PyExc_TypeError, "expected a RakNet object");
        return nullptr;
    }
    PyRakNet* instance = reinterpret_cast<PyRakNet*>(object);
    if (isReceiveThread(instance)) {
        SetApiError(ApiConnectionError, "cannot delete RakNet from its receive callback");
        return nullptr;
    }
    Py_BEGIN_ALLOW_THREADS
    cleanup(instance, true);
    Py_END_ALLOW_THREADS
    Py_RETURN_NONE;
}

PyObject* raknetStartup(PyRakNet* self, PyObject* args)
{
    unsigned short port = 0;
    int maxConnections = 1;
    if (!PyArg_ParseTuple(args, "|Hi", &port, &maxConnections)) return nullptr;
    if (!self->peer) {
        SetApiError(ApiConnectionError, "RakNet instance has been deleted");
        return nullptr;
    }
    if (self->started) {
        SetApiError(ApiConnectionError, "RakNet is already started");
        return nullptr;
    }
    if (maxConnections <= 0) {
        PyErr_SetString(PyExc_ValueError, "max_connections must be positive");
        return nullptr;
    }

    *self->socketDescriptor = SLNet::SocketDescriptor(port, nullptr);
    const SLNet::StartupResult result = self->peer->Startup(maxConnections, self->socketDescriptor, 1);
    if (result != SLNet::RAKNET_STARTED) {
        SetApiError(ApiConnectionError, "RakNet Startup failed with code " + std::to_string(static_cast<int>(result)));
        return nullptr;
    }

    self->peer->SetMaximumIncomingConnections(maxConnections);
    self->started = true;
    self->running->store(true);
    self->recvThread = new std::thread(receiveLoop, self);
    Py_RETURN_NONE;
}

PyObject* raknetConnect(PyRakNet* self, PyObject* args)
{
    const char* host = nullptr;
    unsigned short port = 0;
    const char* password = nullptr;
    Py_ssize_t passwordLength = 0;
    if (!PyArg_ParseTuple(args, "sH|y#", &host, &port, &password, &passwordLength)) return nullptr;
    if (!self->peer || !self->started) {
        SetApiError(ApiConnectionError, "call Startup before Connect");
        return nullptr;
    }

    const SLNet::ConnectionAttemptResult result = self->peer->Connect(
        host, port, password, static_cast<int>(passwordLength));
    if (result != SLNet::CONNECTION_ATTEMPT_STARTED) {
        SetApiError(ApiConnectionError, "RakNet Connect failed with code " + std::to_string(static_cast<int>(result)));
        return nullptr;
    }
    Py_RETURN_NONE;
}

PyObject* raknetSend(PyRakNet* self, PyObject* args)
{
    const char* data = nullptr;
    Py_ssize_t length = 0;
    int priority = MEDIUM_PRIORITY;
    int reliability = UNRELIABLE;
    int orderingChannel = 0;
    if (!PyArg_ParseTuple(args, "y#|iii", &data, &length, &priority, &reliability, &orderingChannel)) {
        return nullptr;
    }
    const SLNet::SystemAddress remoteAddress = getRemoteAddress(self);
    if (!self->peer || !self->started || remoteAddress == SLNet::UNASSIGNED_SYSTEM_ADDRESS) {
        SetApiError(ApiConnectionError, "RakNet is not connected");
        return nullptr;
    }
    if (priority < 0 || priority >= NUMBER_OF_PRIORITIES) {
        PyErr_SetString(PyExc_ValueError, "priority is outside the PacketPriority range");
        return nullptr;
    }
    if (reliability < 0 || reliability >= NUMBER_OF_RELIABILITIES) {
        PyErr_SetString(PyExc_ValueError, "reliability is outside the PacketReliability range");
        return nullptr;
    }
    if (orderingChannel < 0 || orderingChannel > 31) {
        PyErr_SetString(PyExc_ValueError, "ordering_channel must be between 0 and 31");
        return nullptr;
    }

    std::vector<char> buffer(static_cast<size_t>(length) + 1);
    buffer[0] = static_cast<char>(0xfe);
    if (length > 0) std::memcpy(buffer.data() + 1, data, static_cast<size_t>(length));
    const uint32_t receipt = self->peer->Send(
        buffer.data(), static_cast<int>(buffer.size()), static_cast<PacketPriority>(priority),
        static_cast<PacketReliability>(reliability), static_cast<char>(orderingChannel),
        remoteAddress, false);
    return PyLong_FromUnsignedLong(receipt);
}

bool popMessage(PyRakNet* self, int timeoutMs, ReceivedMessage& output)
{
    std::unique_lock<std::mutex> lock(*self->queueMutex);
    if (self->receiveQueue->empty() && timeoutMs > 0) {
        self->queueCondition->wait_for(lock, std::chrono::milliseconds(timeoutMs), [&] {
            return !self->receiveQueue->empty() || !self->running->load();
        });
    }
    if (self->receiveQueue->empty()) return false;
    output = std::move(self->receiveQueue->front());
    self->receiveQueue->pop_front();
    return true;
}

PyObject* raknetReceive(PyRakNet* self, PyObject* args)
{
    int requestedLength = 0;
    int timeoutMs = 0;
    if (!PyArg_ParseTuple(args, "i|i", &requestedLength, &timeoutMs)) return nullptr;
    if (requestedLength < 0 || timeoutMs < 0) {
        PyErr_SetString(PyExc_ValueError, "length and timeout_ms must not be negative");
        return nullptr;
    }

    ReceivedMessage message;
    bool received = false;
    Py_BEGIN_ALLOW_THREADS
    received = popMessage(self, timeoutMs, message);
    Py_END_ALLOW_THREADS

    if (!received) {
        std::string empty(static_cast<size_t>(requestedLength) + 1, '\0');
        return PyBytes_FromStringAndSize(empty.data(), empty.size());
    }

    const size_t bodyLength = requestedLength == 0
        ? message.payload.size()
        : static_cast<size_t>(requestedLength);
    std::string result(bodyLength + 1, '\0');
    result[0] = static_cast<char>(message.messageId);
    const size_t copyLength = std::min(bodyLength, message.payload.size());
    if (copyLength > 0) std::memcpy(result.data() + 1, message.payload.data(), copyLength);
    return PyBytes_FromStringAndSize(result.data(), result.size());
}

PyObject* raknetReceivePacket(PyRakNet* self, PyObject* args)
{
    int timeoutMs = 0;
    if (!PyArg_ParseTuple(args, "|i", &timeoutMs)) return nullptr;
    if (timeoutMs < 0) {
        PyErr_SetString(PyExc_ValueError, "timeout_ms must not be negative");
        return nullptr;
    }

    ReceivedMessage message;
    bool received = false;
    Py_BEGIN_ALLOW_THREADS
    received = popMessage(self, timeoutMs, message);
    Py_END_ALLOW_THREADS
    if (!received) Py_RETURN_NONE;

    PyObject* result = PyDict_New();
    PyObject* value = PyLong_FromUnsignedLong(message.messageId);
    PyDict_SetItemString(result, "message_id", value);
    Py_DECREF(value);
    value = PyBytes_FromStringAndSize(
        reinterpret_cast<const char*>(message.payload.data()), message.payload.size());
    PyDict_SetItemString(result, "payload", value);
    Py_DECREF(value);
    value = PyUnicode_FromStringAndSize(message.address.data(), message.address.size());
    PyDict_SetItemString(result, "address", value);
    Py_DECREF(value);
    return result;
}

PyObject* raknetDisconnect(PyRakNet* self, PyObject*)
{
    if (!self->peer || !self->started) Py_RETURN_NONE;
    const SLNet::SystemAddress remoteAddress = getRemoteAddress(self);
    if (remoteAddress != SLNet::UNASSIGNED_SYSTEM_ADDRESS) {
        self->peer->CloseConnection(remoteAddress, true);
        setRemoteAddress(self, SLNet::UNASSIGNED_SYSTEM_ADDRESS);
    }
    Py_RETURN_NONE;
}

PyObject* raknetShutdown(PyRakNet* self, PyObject* args)
{
    unsigned int blockDuration = 300;
    if (!PyArg_ParseTuple(args, "|I", &blockDuration)) return nullptr;
    if (isReceiveThread(self)) {
        SetApiError(ApiConnectionError, "cannot shut down RakNet from its receive callback");
        return nullptr;
    }
    Py_BEGIN_ALLOW_THREADS
    stopReceiveThread(self);
    if (self->peer && self->started) self->peer->Shutdown(blockDuration);
    Py_END_ALLOW_THREADS
    self->started = false;
    setRemoteAddress(self, SLNet::UNASSIGNED_SYSTEM_ADDRESS);
    Py_RETURN_NONE;
}

PyObject* raknetGetInstanceId(PyRakNet* self, PyObject*)
{
    return PyLong_FromLong(self->instanceId);
}

PyObject* raknetGetConnectionState(PyRakNet* self, PyObject*)
{
    const SLNet::SystemAddress remoteAddress = getRemoteAddress(self);
    if (!self->peer || !self->started || remoteAddress == SLNet::UNASSIGNED_SYSTEM_ADDRESS) {
        return PyLong_FromLong(SLNet::IS_NOT_CONNECTED);
    }
    return PyLong_FromLong(self->peer->GetConnectionState(remoteAddress));
}

PyObject* raknetGetQueueSize(PyRakNet* self, PyObject*)
{
    std::lock_guard<std::mutex> lock(*self->queueMutex);
    return PyLong_FromSize_t(self->receiveQueue->size());
}

PyMethodDef RakNetObjectMethods[] = {
    { "Startup", reinterpret_cast<PyCFunction>(raknetStartup), METH_VARARGS, "Start the RakNet peer." },
    { "Connect", reinterpret_cast<PyCFunction>(raknetConnect), METH_VARARGS, "Begin connecting to a peer." },
    { "Send", reinterpret_cast<PyCFunction>(raknetSend), METH_VARARGS, "Send a 0xfe application message." },
    { "Receive", reinterpret_cast<PyCFunction>(raknetReceive), METH_VARARGS, "Receive legacy status+payload bytes." },
    { "ReceivePacket", reinterpret_cast<PyCFunction>(raknetReceivePacket), METH_VARARGS, "Receive a packet dictionary or None." },
    { "Disconnect", reinterpret_cast<PyCFunction>(raknetDisconnect), METH_NOARGS, "Disconnect the current peer." },
    { "Shutdown", reinterpret_cast<PyCFunction>(raknetShutdown), METH_VARARGS, "Stop the RakNet peer." },
    { "GetInstanceId", reinterpret_cast<PyCFunction>(raknetGetInstanceId), METH_NOARGS, "Return the wrapper instance ID." },
    { "GetConnectionState", reinterpret_cast<PyCFunction>(raknetGetConnectionState), METH_NOARGS, "Return the SLikeNet connection state." },
    { "GetReceiveQueueSize", reinterpret_cast<PyCFunction>(raknetGetQueueSize), METH_NOARGS, "Return queued packet count." },
    { nullptr, nullptr, 0, nullptr }
};

PyMethodDef RakNetModuleMethods[] = {
    { "get_raknet", raknetGet, METH_NOARGS, "Create a RakNet instance." },
    { "delete", raknetDelete, METH_VARARGS, "Release a RakNet instance's network resources." },
    { nullptr, nullptr, 0, nullptr }
};

PyModuleDef RakNetModule = {
    PyModuleDef_HEAD_INIT,
    "_raknet",
    "SLikeNet network wrapper",
    -1,
    RakNetModuleMethods
};

} // namespace

extern "C" PyMODINIT_FUNC PyInit__raknet(void)
{
    PyRakNet_Type.tp_name = "_raknet.RakNet";
    PyRakNet_Type.tp_basicsize = sizeof(PyRakNet);
    PyRakNet_Type.tp_flags = Py_TPFLAGS_DEFAULT;
    PyRakNet_Type.tp_doc = "RakNet peer wrapper";
    PyRakNet_Type.tp_methods = RakNetObjectMethods;
    PyRakNet_Type.tp_new = raknetNew;
    PyRakNet_Type.tp_dealloc = reinterpret_cast<destructor>(raknetDealloc);
    if (PyType_Ready(&PyRakNet_Type) < 0) return nullptr;

    PyObject* module = PyModule_Create(&RakNetModule);
    if (!module) return nullptr;
    Py_INCREF(&PyRakNet_Type);
    if (PyModule_AddObject(module, "RakNet", reinterpret_cast<PyObject*>(&PyRakNet_Type)) < 0) {
        Py_DECREF(&PyRakNet_Type);
        Py_DECREF(module);
        return nullptr;
    }

    PyModule_AddIntConstant(module, "IMMEDIATE_PRIORITY", IMMEDIATE_PRIORITY);
    PyModule_AddIntConstant(module, "HIGH_PRIORITY", HIGH_PRIORITY);
    PyModule_AddIntConstant(module, "MEDIUM_PRIORITY", MEDIUM_PRIORITY);
    PyModule_AddIntConstant(module, "LOW_PRIORITY", LOW_PRIORITY);
    PyModule_AddIntConstant(module, "UNRELIABLE", UNRELIABLE);
    PyModule_AddIntConstant(module, "UNRELIABLE_SEQUENCED", UNRELIABLE_SEQUENCED);
    PyModule_AddIntConstant(module, "RELIABLE", RELIABLE);
    PyModule_AddIntConstant(module, "RELIABLE_ORDERED", RELIABLE_ORDERED);
    PyModule_AddIntConstant(module, "RELIABLE_SEQUENCED", RELIABLE_SEQUENCED);
    Py_INCREF(ApiConnectionError);
    PyModule_AddObject(module, "ConnectionError", ApiConnectionError);
    return module;
}
