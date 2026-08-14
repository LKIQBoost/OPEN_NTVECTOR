// _websocket.cpp
#include <Python.h>
#include <string>
#include <cstring>
#include <memory>
#include "WebSockeVirtualWrapper.h"
#include "engine_wrapper.h"

class EventWebSocketWrapper {
public:
    int _EventID;
	void onDataReceived(const std::string& content, size_t) {
		Logger::getInstance().log(LOG_INFO, "WebSocketMessage: ", content);
		Logger::getInstance().log(LOG_INFO, "_websocket python event wrapper: on_message");
		PythonEventEngine e;
		e.trigger("websocket_message", _EventID, content);
	}
	void onConnection(bool connected) {
		Logger::getInstance().log(LOG_INFO, "_websocket python event wrapper: on_connection");
		PythonEventEngine e;
		e.trigger("websocket_connection", _EventID, connected);
	}
private:
};


// ǰ������
static void WebSocket_dealloc(PyObject* self);
static PyObject* WebSocket_new(PyTypeObject* type, PyObject* args, PyObject* kwds);
static int WebSocket_init(PyObject* self, PyObject* args, PyObject* kwds);
static PyObject* WebSocket_connect(PyObject* self, PyObject* args);
static PyObject* WebSocket_send(PyObject* self, PyObject* args);
static PyObject* WebSocket_close(PyObject* self, PyObject* args);
static PyObject* WebSocket_get_id(PyObject* self, void* closure);

// �Ƴ�ȫ��map��mutex����Ϊ����self�У�
static int g_next_id = 1000;
static int _self_event_id = 0;
PythonEventEngine g_event_engine;

// ����ΨһID
static int generate_id() {
    return g_next_id++;
}

// WebSocket����ṹ�������޸ģ�ֱ�Ӵ洢WebSocketClientָ�룩
typedef struct {
    PyObject_HEAD
        int id;
    char* ip;
    int port;
    char* path;
    // �����޸ģ���WebSocketClientֱ�Ӵ��ڶ����У����ȫ��map
    WebSocketClient* ws_client;  // �洢�ͻ���ʵ��
    EventWebSocketWrapper* ws_event_wrapper;  //�洢�ͻ����¼���װ
} WebSocketObject;


// ��������
static PyMethodDef WebSocket_methods[] = {
    {"connect", (PyCFunction)WebSocket_connect, METH_VARARGS, "Connect to server"},
    {"send", (PyCFunction)WebSocket_send, METH_VARARGS, "Send data"},
    {"close", (PyCFunction)WebSocket_close, METH_VARARGS, "Close connection"},
    {NULL, NULL, 0, NULL}
};

// ���Զ���
static PyGetSetDef WebSocket_getsetters[] = {
    {(char*)"event_id", (getter)WebSocket_get_id, NULL, (char*)"Event ID", NULL},
    {NULL}
};

// PyTypeObject (Python 3) - fields are assigned in PyInit__websocket to avoid
// depending on the exact 3.12 struct layout.
static PyTypeObject WebSocketType = { PyVarObject_HEAD_INIT(NULL, 0) };

// ���������������޸ģ��ͷ�self�е�WebSocketClient��
static void WebSocket_dealloc(PyObject* self) {
    WebSocketObject* ws_obj = (WebSocketObject*)self;

    // �ͷ�WebSocketClientʵ��
    if (ws_obj->ws_client) {
        delete ws_obj->ws_client;
        ws_obj->ws_client = nullptr;
    }

    // �ͷ��ַ�����Դ
    if (ws_obj->ip) {
        free(ws_obj->ip);
        ws_obj->ip = NULL;
    }
    if (ws_obj->path) {
        free(ws_obj->path);
        ws_obj->path = NULL;
    }

    // ����Python���õ��ͷ��߼�
    Py_TYPE(self)->tp_free(self);
}

// ���캯��
static PyObject* WebSocket_new(PyTypeObject* type, PyObject* args, PyObject* kwds) {
    WebSocketObject* self = (WebSocketObject*)type->tp_alloc(type, 0);
    if (self) {
        self->id = 0;
        self->ip = NULL;
        self->port = 0;
        self->path = NULL;
        self->ws_client = nullptr;  // ��ʼ���ͻ���ָ��
        self->ws_event_wrapper = nullptr;  // ��ʼ���ͻ���ָ��
    }
    return (PyObject*)self;
}

// ��ʼ�������������޸ģ�����WebSocketClient������self��
static int WebSocket_init(PyObject* self, PyObject* args, PyObject* kwds) {
    WebSocketObject* ws_obj = (WebSocketObject*)self;
    const char* ip = NULL;
    int port = 0;
    const char* path = NULL;

    // ��������
    if (!PyArg_ParseTuple(args, "sis", &ip, &port, &path)) {
        PyErr_SetString(PyExc_TypeError, "Expected: (str ip, int port, str path)");
        return -1;
    }

    // ��ֵ��������
    ws_obj->ip = strdup(ip);
    ws_obj->port = port;
    ws_obj->path = strdup(path);
    ws_obj->id = generate_id();

    // �����޸ģ�����WebSocketClient������self�����ȫ��map��
    ws_obj->ws_client = new WebSocketClient(ip, port, "Minecraft-Bedrock", path);
    ws_obj->ws_event_wrapper = new EventWebSocketWrapper()
    ;
    ws_obj->ws_event_wrapper->_EventID = ws_obj->id;

    return 0;
}

// �޸� WebSocket_connect �����е� std::bind ������ȷ������ƥ��  
static PyObject* WebSocket_connect(PyObject* self, PyObject* args) {  
   WebSocketObject* ws_obj = (WebSocketObject*)self;  

   // �����ղ���  
   if (!PyArg_ParseTuple(args, "")) {  
       return NULL;  
   }  

   // ���ͻ����Ƿ����  
   if (!ws_obj->ws_client || !(ws_obj->ws_client)) {  
       PyErr_SetString(PyExc_RuntimeError, "WebSocket client not initialized");  
       return NULL;  
   }  

   // �����޸ģ�ʹ�� lambda ��װ std::bind ��ȷ������ƥ��  
   auto onDataReceivedWrapper = [wrapper = ws_obj->ws_event_wrapper](const std::string& content, size_t size) {  
       wrapper->onDataReceived(content, size);  
   };  

   (ws_obj->ws_client)->connect(  
       onDataReceivedWrapper,  
       std::bind(&EventWebSocketWrapper::onConnection, ws_obj->ws_event_wrapper, std::placeholders::_1)  
   );  

   // ��Ҫ��ֱ�ӷ��� 0����Ӧ Python �� False��  
   return PyBool_FromLong(0);  
}

// send����������self�洢�Ŀͻ��ˣ�
static PyObject* WebSocket_send(PyObject* self, PyObject* args) {
    WebSocketObject* ws_obj = (WebSocketObject*)self;
    const char* data = NULL;
    int len = 0;

    // ��������
    if (!PyArg_ParseTuple(args, "s#", &data, &len)) {
        return NULL;
    }

    // ���ͻ����Ƿ����
    if (!ws_obj->ws_client || !(ws_obj->ws_client)) {
        PyErr_SetString(PyExc_RuntimeError, "WebSocket client not found");
        return NULL;
    }

    // ��������
    bool result = (ws_obj->ws_client)->sendData(std::string(data, len));
    return PyBool_FromLong(result ? 1 : 0);
}

// close����������self�洢�Ŀͻ��ˣ�
static PyObject* WebSocket_close(PyObject* self, PyObject* args) {
    WebSocketObject* ws_obj = (WebSocketObject*)self;

    // �����ղ���
    if (!PyArg_ParseTuple(args, "")) {
        return NULL;
    }

    // �ر�����
    if (ws_obj->ws_client && (ws_obj->ws_client)) {
        (ws_obj->ws_client)->disconnect();
    }

    Py_RETURN_NONE;
}

// ��ȡevent_id����
static PyObject* WebSocket_get_id(PyObject* self, void* closure) {
    WebSocketObject* ws_obj = (WebSocketObject*)self;
    return PyLong_FromLong(ws_obj->id);
}

// ģ�鼶������get_websocket
static PyObject* get_websocket(PyObject* self, PyObject* args) {

    //std::unique_ptr<WebSocketClient> wsc = std::make_unique<WebSocketClient>("45.253.177.75", 8899, "Minecraft-Bedrock", "/4034328500471339769/2881323365/1MYbM/W6znswDilR/8NBNA==/lqb546Yzvm5HL93jAL3BZw==");
    //wsc->connect();
    //cin.get();

    const char* ip = NULL;
    int port = 0;
    const char* path = NULL;

    if (!PyArg_ParseTuple(args, "sis", &ip, &port, &path)) {
        return NULL;
    }

    PyObject* arg_tuple = Py_BuildValue("(sis)", ip, port, path);
    PyObject* obj = PyObject_CallObject((PyObject*)&WebSocketType, arg_tuple);
    Py_DECREF(arg_tuple);

    return obj;
}

// ģ�鼶������delete������self�洢�Ŀͻ��ˣ�
static PyObject* delete_websocket(PyObject* self, PyObject* args) {
    PyObject* obj = NULL;

    if (!PyArg_ParseTuple(args, "O", &obj)) {
        return NULL;
    }

    // �������
    if (!PyObject_TypeCheck(obj, &WebSocketType)) {
        PyErr_SetString(PyExc_TypeError, "Expected WebSocket object");
        return NULL;
    }

    WebSocketObject* ws_obj = (WebSocketObject*)obj;

    // �ͷſͻ���ʵ��
    if (ws_obj->ws_client) {
        delete ws_obj->ws_client;
        ws_obj->ws_client = nullptr;
    }
    if (ws_obj->ws_event_wrapper) {
        delete ws_obj->ws_event_wrapper;
        ws_obj->ws_event_wrapper = nullptr;
    }

    Py_RETURN_NONE;
}

// ģ�鷽����
static PyMethodDef ModuleMethods[] = {
    {"get_websocket", get_websocket, METH_VARARGS, "Create WebSocket"},
    {"delete", delete_websocket, METH_VARARGS, "Delete WebSocket"},
    {NULL, NULL, 0, NULL}
};

// Module init (Python 3)
static struct PyModuleDef _websocket_module = { PyModuleDef_HEAD_INIT, "_websocket", NULL, -1, ModuleMethods };
PyMODINIT_FUNC PyInit__websocket(void) {
    WebSocketType.tp_name = "_websocket.WebSocket";
    WebSocketType.tp_basicsize = sizeof(WebSocketObject);
    WebSocketType.tp_dealloc = (destructor)WebSocket_dealloc;
    WebSocketType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
    WebSocketType.tp_doc = "WebSocket client";
    WebSocketType.tp_methods = WebSocket_methods;
    WebSocketType.tp_getset = WebSocket_getsetters;
    WebSocketType.tp_init = (initproc)WebSocket_init;
    WebSocketType.tp_new = WebSocket_new;
    if (PyType_Ready(&WebSocketType) < 0) {
        return NULL;
    }
    PyObject* module = PyModule_Create(&_websocket_module);
    if (!module) {
        return NULL;
    }
    Py_INCREF(&WebSocketType);
    PyModule_AddObject(module, "WebSocket", (PyObject*)&WebSocketType);
    return module;
}