#include <Python.h>
#include <thread>
#include <atomic>
#include <chrono>
#include <SLikeNet/BitStream.h>
#include <MessageIdentifiers.h>
#include <RakPeerInterface.h>
#include <RakNetTypes.h>
#include <RakSleep.h>
#include "engine_wrapper.h"
#include "Logger.h"

#ifdef __cplusplus
extern "C" {
#endif

    // ǰ������
    extern PyTypeObject PyRakNet_Type;

    // RakNet�����Python��װ�ṹ
    typedef struct {
        PyObject_HEAD
            SLNet::RakPeerInterface* peer;
        SLNet::SystemAddress systemAddress;
        SLNet::SocketDescriptor socketDescriptor;
        std::thread* recv_thread;
        std::atomic<bool>* running;
        int instance_id;
    } PyRakNet;

    static int g_next_instance_id = 1;

    // �����̺߳���
    void raknet_recv_thread_func(PyRakNet* self) {
        Logger::getInstance().log(LOG_RAKNET, std::string() + "[RakNet] Recv thread started, instance_id: " + std::to_string(self->instance_id));

        while (self->running->load()) {
            if (!self->peer) {
                std::this_thread::sleep_for(std::chrono::milliseconds(15));
                continue;
            }

            // �������д����������ݰ�
            SLNet::Packet* packet;
            while (self->running->load() && (packet = self->peer->Receive())) {
                unsigned char messageId = packet->data[0];

                switch (messageId) {
                case ID_CONNECTION_REQUEST_ACCEPTED:
                    Logger::getInstance().log_("ID_CONNECTION_REQUEST_ACCEPTED", LOG_NETWORK);
                    self->systemAddress = packet->systemAddress;

                    Logger::getInstance().log(LOG_RAKNET, std::string() + "[RakNet] Connection accepted, instance_id: " + std::to_string(self->instance_id) +
                        ", address: " + packet->systemAddress.ToString(false));

                    {
                        PythonEventEngine engine;
                        engine.trigger("ID_CONNECTION_REQUEST_ACCEPTED",
                            self->instance_id,
                            packet->systemAddress.ToString(false));
                    }
                    break;

                case ID_DISCONNECTION_NOTIFICATION:
                    Logger::getInstance().log_("ID_DISCONNECTION_NOTIFICATION", LOG_NETWORK);
                    Logger::getInstance().log(LOG_RAKNET, std::string() + "[RakNet] Disconnected, instance_id: " + std::to_string(self->instance_id));
                    {
                        PythonEventEngine engine;
                        engine.trigger("ID_DISCONNECTION_NOTIFICATION", self->instance_id);
                    }
                    self->systemAddress = SLNet::UNASSIGNED_SYSTEM_ADDRESS;
                    break;

                case ID_CONNECTION_LOST:
                    Logger::getInstance().log_("ID_CONNECTION_LOST", LOG_NETWORK);
                    Logger::getInstance().log(LOG_RAKNET, std::string() + "[RakNet] Connection lost, instance_id: " + std::to_string(self->instance_id));
                    {
                        PythonEventEngine engine;
                        engine.trigger("ID_CONNECTION_LOST", self->instance_id);
                    }
                    self->systemAddress = SLNet::UNASSIGNED_SYSTEM_ADDRESS;
                    break;

                case ID_CONNECTION_ATTEMPT_FAILED:
                    Logger::getInstance().log_("ID_CONNECTION_ATTEMPT_FAILED", LOG_NETWORK);
                    Logger::getInstance().log(LOG_RAKNET, std::string() + "[RakNet] Connection attempt failed, instance_id: " + std::to_string(self->instance_id));
                    {
                        PythonEventEngine engine;
                        engine.trigger("ID_CONNECTION_ATTEMPT_FAILED", self->instance_id);
                    }
                    self->systemAddress = SLNet::UNASSIGNED_SYSTEM_ADDRESS;
                    break;

                case 0xfe:  // �Զ���Э�����ݰ�
                    Logger::getInstance().log(LOG_RAKNET, std::string() + "[RakNet] Message received, instance_id: " + std::to_string(self->instance_id) +
                        ", length: " + std::to_string(packet->length - 1));
                    {
                        PythonEventEngine engine;
                        engine.trigger("on_raknet_message",
                            self->instance_id,
                            std::string((char*)packet->data + 1, packet->length - 1),
                            packet->systemAddress.ToString(false));
                    }
                    break;

                default:
                    // ����������Ϣ������¼��־����ˢ��
                    break;
                }

                self->peer->DeallocatePacket(packet);
            }

            // 15ms һ֡��Լ60FPS
            std::this_thread::sleep_for(std::chrono::milliseconds(15));
        }

        Logger::getInstance().log(LOG_RAKNET, std::string() + "[RakNet] Recv thread exited, instance_id: " + std::to_string(self->instance_id));
    }

    // ɾ������
    static PyObject* raknet_delete(PyObject* self, PyObject* args) {
        PyObject* py_obj = NULL;
        if (!PyArg_ParseTuple(args, "O", &py_obj))
            return NULL;

        if (Py_TYPE(py_obj) != &PyRakNet_Type) {
            PyErr_SetString(PyExc_TypeError, "Expected RakNet object");
            return NULL;
        }

        PyRakNet* raknet_obj = (PyRakNet*)py_obj;

        // ֹͣ�����߳�
        if (raknet_obj->running) {
            raknet_obj->running->store(false);
        }

        // �ȴ��߳̽���
        if (raknet_obj->recv_thread && raknet_obj->recv_thread->joinable()) {
            raknet_obj->recv_thread->join();
            delete raknet_obj->recv_thread;
            raknet_obj->recv_thread = nullptr;
        }

        // �ͷ�RakPeer
        if (raknet_obj->peer) {
            raknet_obj->peer->Shutdown(300);
            SLNet::RakPeerInterface::DestroyInstance(raknet_obj->peer);
            raknet_obj->peer = NULL;
        }

        // �ͷ�running��־
        if (raknet_obj->running) {
            delete raknet_obj->running;
            raknet_obj->running = nullptr;
        }

        Py_RETURN_NONE;
    }

    // ��ȡRakNetʵ��
    static PyObject* raknet_get_raknet(PyObject* self, PyObject* args) {
        PyRakNet* raknet_obj = PyObject_New(PyRakNet, &PyRakNet_Type);
        if (!raknet_obj)
            return NULL;

        raknet_obj->peer = SLNet::RakPeerInterface::GetInstance();
        raknet_obj->systemAddress = SLNet::UNASSIGNED_SYSTEM_ADDRESS;
        raknet_obj->instance_id = g_next_instance_id++;
        raknet_obj->recv_thread = nullptr;
        raknet_obj->running = new std::atomic<bool>(false);

        Logger::getInstance().log(LOG_RAKNET, std::string() + "[RakNet] Instance created, ID: " + std::to_string(raknet_obj->instance_id));

        return (PyObject*)raknet_obj;
    }

    // Startup - ��ʼ��
    static PyObject* raknet_startup(PyRakNet* self, PyObject* args) {
        unsigned short port = 0;
        int maxConnections = 1;

        if (!PyArg_ParseTuple(args, "|Hi", &port, &maxConnections))
            return NULL;

        if (!self->peer) {
            PyErr_SetString(PyExc_RuntimeError, "RakPeer not initialized");
            return NULL;
        }

        self->socketDescriptor = SLNet::SocketDescriptor(port, 0);
        SLNet::StartupResult result = self->peer->Startup(maxConnections, &self->socketDescriptor, 1);

        if (result != SLNet::RAKNET_STARTED) {
            PyErr_SetString(PyExc_RuntimeError, "Failed to startup RakNet");
            return NULL;
        }

        self->peer->SetMaximumIncomingConnections(maxConnections);

        // ���������߳�
        if (!self->recv_thread) {
            self->running->store(true);
            self->recv_thread = new std::thread(raknet_recv_thread_func, self);
            self->recv_thread->detach();
            Logger::getInstance().log(LOG_RAKNET, std::string() + "[RakNet] Recv thread started, instance_id: " + std::to_string(self->instance_id));
        }

        Py_RETURN_NONE;
    }

    // Connect - ���ӷ�����
    static PyObject* raknet_connect(PyRakNet* self, PyObject* args) {
        const char* host;
        unsigned short port;
        const char* password = NULL;
        int passwordLen = 0;

        if (!PyArg_ParseTuple(args, "sH|s#", &host, &port, &password, &passwordLen))
            return NULL;

        if (!self->peer) {
            PyErr_SetString(PyExc_RuntimeError, "RakPeer not initialized");
            return NULL;
        }

        SLNet::ConnectionAttemptResult result;
        if (password && passwordLen > 0) {
            result = self->peer->Connect(host, port, password, passwordLen);
        }
        else {
            result = self->peer->Connect(host, port, 0, 0);
        }

        if (result != SLNet::CONNECTION_ATTEMPT_STARTED) {
            PyErr_SetString(PyExc_RuntimeError, "Failed to connect");
            return NULL;
        }

        Logger::getInstance().log(LOG_RAKNET, std::string() + "[RakNet] Connecting to " + host + ":" + std::to_string(port) +
            ", instance_id: " + std::to_string(self->instance_id));
        Py_RETURN_NONE;
    }

    // Send - �������ݣ��Զ�����0xfeͷ
    static PyObject* raknet_send(PyRakNet* self, PyObject* args) {
        const char* data;
        int length;
        int priority = 0;      // MEDIUM_PRIORITY
        int reliability = 2;   // UNRELIABLE
        char orderingChannel = 0;

        if (!PyArg_ParseTuple(args, "s#|iib", &data, &length, &priority, &reliability, &orderingChannel))
            return NULL;

        if (!self->peer) {
            PyErr_SetString(PyExc_RuntimeError, "RakPeer not initialized");
            return NULL;
        }

        if (self->systemAddress == SLNet::UNASSIGNED_SYSTEM_ADDRESS) {
            PyErr_SetString(PyExc_RuntimeError, "Not connected to any server");
            return NULL;
        }

        // ���仺����: 1�ֽ���ϢID(0xfe) + ����
        char* send_buffer = (char*)PyMem_Malloc(length + 1);
        if (!send_buffer) {
            PyErr_SetString(PyExc_MemoryError, "Failed to allocate send buffer");
            return NULL;
        }

        send_buffer[0] = (char)0xfe;
        memcpy(send_buffer + 1, data, length);

        uint32_t result = self->peer->Send(
            send_buffer,
            length + 1,
            MEDIUM_PRIORITY,
            UNRELIABLE,
            0,
            self->systemAddress,
            false
        );

        PyMem_Free(send_buffer);
        Logger::getInstance().log(LOG_RAKNET, std::string() + "[RakNet] Sent " + std::to_string(length) +
            " bytes, instance_id: " + std::to_string(self->instance_id));
        return PyInt_FromLong(result);
    }

    // Receive - ����ͬ���ӿڣ���ֻ����״̬
    static PyObject* raknet_receive(PyRakNet* self, PyObject* args) {
        int requestedLength;
        if (!PyArg_ParseTuple(args, "i", &requestedLength))
            return NULL;

        char* result = (char*)PyMem_Malloc(requestedLength + 1);
        if (!result) {
            PyErr_SetString(PyExc_MemoryError, "Failed to allocate memory");
            return NULL;
        }

        result[0] = 0;  // ״̬0�������ݣ�����ʹ�ûص���
        memset(result + 1, 0, requestedLength);

        PyObject* ret = PyString_FromStringAndSize(result, requestedLength + 1);
        PyMem_Free(result);

        return ret;
    }

    // Disconnect - �Ͽ�����
    static PyObject* raknet_disconnect(PyRakNet* self, PyObject* args) {
        if (!self->peer) {
            PyErr_SetString(PyExc_RuntimeError, "RakPeer not initialized");
            return NULL;
        }

        self->peer->CloseConnection(self->systemAddress, true);
        self->systemAddress = SLNet::UNASSIGNED_SYSTEM_ADDRESS;
        Logger::getInstance().log(LOG_RAKNET, std::string() + "[RakNet] Disconnected, instance_id: " + std::to_string(self->instance_id));

        Py_RETURN_NONE;
    }

    // Shutdown - �ر�
    static PyObject* raknet_shutdown(PyRakNet* self, PyObject* args) {
        unsigned int blockDuration = 300;

        if (!PyArg_ParseTuple(args, "|I", &blockDuration))
            return NULL;

        if (!self->peer) {
            PyErr_SetString(PyExc_RuntimeError, "RakPeer not initialized");
            return NULL;
        }

        // ֹͣ�����߳�
        if (self->running) {
            self->running->store(false);
        }

        self->peer->Shutdown(blockDuration);
        self->systemAddress = SLNet::UNASSIGNED_SYSTEM_ADDRESS;
        Logger::getInstance().log(LOG_RAKNET, std::string() + "[RakNet] Shutdown, instance_id: " + std::to_string(self->instance_id));

        Py_RETURN_NONE;
    }

    // ��ȡʵ��ID
    static PyObject* raknet_get_instance_id(PyRakNet* self, PyObject* args) {
        return PyInt_FromLong(self->instance_id);
    }

    // ��������
    static PyMethodDef PyRakNet_methods[] = {
        {"Startup", (PyCFunction)raknet_startup, METH_VARARGS, "Startup RakNet"},
        {"Connect", (PyCFunction)raknet_connect, METH_VARARGS, "Connect to server"},
        {"Send", (PyCFunction)raknet_send, METH_VARARGS, "Send data with 0xfe header"},
        {"Receive", (PyCFunction)raknet_receive, METH_VARARGS, "Receive data (sync, use callbacks instead)"},
        {"Disconnect", (PyCFunction)raknet_disconnect, METH_VARARGS, "Disconnect"},
        {"Shutdown", (PyCFunction)raknet_shutdown, METH_VARARGS, "Shutdown"},
        {"GetInstanceId", (PyCFunction)raknet_get_instance_id, METH_NOARGS, "Get instance ID"},
        {NULL, NULL, 0, NULL}
    };

    // ���Ͷ���
    PyTypeObject PyRakNet_Type = {
        PyObject_HEAD_INIT(NULL)
        0,                         // ob_size
        "_raknet.RakNet",          // tp_name
        sizeof(PyRakNet),          // tp_basicsize
        0,                         // tp_itemsize
        (destructor)0,            // tp_dealloc
        0,                         // tp_print
        0,                         // tp_getattr
        0,                         // tp_setattr
        0,                         // tp_compare
        0,                         // tp_repr
        0,                         // tp_as_number
        0,                         // tp_as_sequence
        0,                         // tp_as_mapping
        0,                         // tp_hash
        0,                         // tp_call
        0,                         // tp_str
        0,                         // tp_getattro
        0,                         // tp_setattro
        0,                         // tp_as_buffer
        Py_TPFLAGS_DEFAULT,        // tp_flags
        "RakNet wrapper object",   // tp_doc
        0,                         // tp_traverse
        0,                         // tp_clear
        0,                         // tp_richcompare
        0,                         // tp_weaklistoffset
        0,                         // tp_iter
        0,                         // tp_iternext
        PyRakNet_methods,          // tp_methods
        0,                         // tp_members
        0,                         // tp_getset
        0,                         // tp_base
        0,                         // tp_dict
        0,                         // tp_descr_get
        0,                         // tp_descr_set
        0,                         // tp_dictoffset
        0,                         // tp_init
        0,                         // tp_alloc
        0,                         // tp_new
    };

    // ģ�鷽��
    static PyMethodDef RakNetMethods[] = {
        {"get_raknet", raknet_get_raknet, METH_VARARGS, "Create RakNet instance"},
        {"delete", raknet_delete, METH_VARARGS, "Delete RakNet instance"},
        {NULL, NULL, 0, NULL}
    };

    // ģ���ʼ������
    void init_raknet(void) {
        // ��ʼ������
        PyRakNet_Type.tp_new = PyType_GenericNew;
        PyRakNet_Type.tp_dealloc = (destructor)raknet_delete;
        if (PyType_Ready(&PyRakNet_Type) < 0)
            return;

        // ����ģ��
        PyObject* module = Py_InitModule("_raknet", RakNetMethods);
        if (!module)
            return;

        // �������͵�ģ��
        Py_INCREF(&PyRakNet_Type);
        PyModule_AddObject(module, "RakNet", (PyObject*)&PyRakNet_Type);

        Logger::getInstance().log(LOG_RAKNET, "[RakNet] Module initialized");
    }

#ifdef __cplusplus
}
#endif