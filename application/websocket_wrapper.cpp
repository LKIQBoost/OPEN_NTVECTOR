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


// 前向声明
static void WebSocket_dealloc(PyObject* self);
static PyObject* WebSocket_new(PyTypeObject* type, PyObject* args, PyObject* kwds);
static int WebSocket_init(PyObject* self, PyObject* args, PyObject* kwds);
static PyObject* WebSocket_connect(PyObject* self, PyObject* args);
static PyObject* WebSocket_send(PyObject* self, PyObject* args);
static PyObject* WebSocket_close(PyObject* self, PyObject* args);
static PyObject* WebSocket_get_id(PyObject* self, void* closure);

// 移除全局map和mutex（改为存在self中）
static int g_next_id = 1000;
static int _self_event_id = 0;
PythonEventEngine g_event_engine;

// 生成唯一ID
static int generate_id() {
    return g_next_id++;
}

// WebSocket对象结构（核心修改：直接存储WebSocketClient指针）
typedef struct {
    PyObject_HEAD
        int id;
    char* ip;
    int port;
    char* path;
    // 核心修改：将WebSocketClient直接存在对象中，替代全局map
    WebSocketClient* ws_client;  // 存储客户端实例
    EventWebSocketWrapper* ws_event_wrapper;  //存储客户端事件包装
} WebSocketObject;


// 方法定义
static PyMethodDef WebSocket_methods[] = {
    {"connect", (PyCFunction)WebSocket_connect, METH_VARARGS, "Connect to server"},
    {"send", (PyCFunction)WebSocket_send, METH_VARARGS, "Send data"},
    {"close", (PyCFunction)WebSocket_close, METH_VARARGS, "Close connection"},
    {NULL, NULL, 0, NULL}
};

// 属性定义
static PyGetSetDef WebSocket_getsetters[] = {
    {(char*)"event_id", (getter)WebSocket_get_id, NULL, (char*)"Event ID", NULL},
    {NULL}
};

// 完整的PyTypeObject定义（Python 2.7格式）
static PyTypeObject WebSocketType = {
    PyObject_HEAD_INIT(NULL)           // PyObject_VAR_HEAD
    0,                                  // ob_size (deprecated)
    "_websocket.WebSocket",              // tp_name
    sizeof(WebSocketObject),             // tp_basicsize
    0,                                  // tp_itemsize
    (destructor)WebSocket_dealloc,       // tp_dealloc
    0,                                  // tp_print
    0,                                  // tp_getattr
    0,                                  // tp_setattr
    0,                                  // tp_compare
    0,                                  // tp_repr
    0,                                  // tp_as_number
    0,                                  // tp_as_sequence
    0,                                  // tp_as_mapping
    0,                                  // tp_hash
    0,                                  // tp_call
    0,                                  // tp_str
    0,                                  // tp_getattro
    0,                                  // tp_setattro
    0,                                  // tp_as_buffer
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,  // tp_flags
    "WebSocket client",                  // tp_doc
    0,                                  // tp_traverse
    0,                                  // tp_clear
    0,                                  // tp_richcompare
    0,                                  // tp_weaklistoffset
    0,                                  // tp_iter
    0,                                  // tp_iternext
    WebSocket_methods,                   // tp_methods
    0,                                  // tp_members
    WebSocket_getsetters,                // tp_getset
    0,                                  // tp_base
    0,                                  // tp_dict
    0,                                  // tp_descr_get
    0,                                  // tp_descr_set
    0,                                  // tp_dictoffset
    (initproc)WebSocket_init,            // tp_init
    0,                                  // tp_alloc
    WebSocket_new,                       // tp_new
    0,                                  // tp_free (Python will set)
    0,                                  // tp_is_gc
    0,                                  // tp_bases
    0,                                  // tp_mro
    0,                                  // tp_cache
    0,                                  // tp_subclasses
    0,                                  // tp_weaklist
    0,                                  // tp_del
    0,                                  // tp_version_tag
};

// 析构函数（核心修改：释放self中的WebSocketClient）
static void WebSocket_dealloc(PyObject* self) {
    WebSocketObject* ws_obj = (WebSocketObject*)self;

    // 释放WebSocketClient实例
    if (ws_obj->ws_client) {
        delete ws_obj->ws_client;
        ws_obj->ws_client = nullptr;
    }

    // 释放字符串资源
    if (ws_obj->ip) {
        free(ws_obj->ip);
        ws_obj->ip = NULL;
    }
    if (ws_obj->path) {
        free(ws_obj->path);
        ws_obj->path = NULL;
    }

    // 调用Python内置的释放逻辑
    Py_TYPE(self)->tp_free(self);
}

// 构造函数
static PyObject* WebSocket_new(PyTypeObject* type, PyObject* args, PyObject* kwds) {
    WebSocketObject* self = (WebSocketObject*)type->tp_alloc(type, 0);
    if (self) {
        self->id = 0;
        self->ip = NULL;
        self->port = 0;
        self->path = NULL;
        self->ws_client = nullptr;  // 初始化客户端指针
        self->ws_event_wrapper = nullptr;  // 初始化客户端指针
    }
    return (PyObject*)self;
}

// 初始化函数（核心修改：创建WebSocketClient并存入self）
static int WebSocket_init(PyObject* self, PyObject* args, PyObject* kwds) {
    WebSocketObject* ws_obj = (WebSocketObject*)self;
    const char* ip = NULL;
    int port = 0;
    const char* path = NULL;

    // 解析参数
    if (!PyArg_ParseTuple(args, "sis", &ip, &port, &path)) {
        PyErr_SetString(PyExc_TypeError, "Expected: (str ip, int port, str path)");
        return -1;
    }

    // 赋值基础属性
    ws_obj->ip = strdup(ip);
    ws_obj->port = port;
    ws_obj->path = strdup(path);
    ws_obj->id = generate_id();

    // 核心修改：创建WebSocketClient并存入self（替代全局map）
    ws_obj->ws_client = new WebSocketClient(ip, port, "Minecraft-Bedrock", path);
    ws_obj->ws_event_wrapper = new EventWebSocketWrapper()
    ;
    ws_obj->ws_event_wrapper->_EventID = ws_obj->id;

    return 0;
}

// 修改 WebSocket_connect 方法中的 std::bind 调用以确保类型匹配  
static PyObject* WebSocket_connect(PyObject* self, PyObject* args) {  
   WebSocketObject* ws_obj = (WebSocketObject*)self;  

   // 解析空参数  
   if (!PyArg_ParseTuple(args, "")) {  
       return NULL;  
   }  

   // 检查客户端是否存在  
   if (!ws_obj->ws_client || !(ws_obj->ws_client)) {  
       PyErr_SetString(PyExc_RuntimeError, "WebSocket client not initialized");  
       return NULL;  
   }  

   // 核心修改：使用 lambda 包装 std::bind 以确保类型匹配  
   auto onDataReceivedWrapper = [wrapper = ws_obj->ws_event_wrapper](const std::string& content, size_t size) {  
       wrapper->onDataReceived(content, size);  
   };  

   (ws_obj->ws_client)->connect(  
       onDataReceivedWrapper,  
       std::bind(&EventWebSocketWrapper::onConnection, ws_obj->ws_event_wrapper, std::placeholders::_1)  
   );  

   // 按要求直接返回 0（对应 Python 的 False）  
   return PyBool_FromLong(0);  
}

// send方法（适配self存储的客户端）
static PyObject* WebSocket_send(PyObject* self, PyObject* args) {
    WebSocketObject* ws_obj = (WebSocketObject*)self;
    const char* data = NULL;
    int len = 0;

    // 解析参数
    if (!PyArg_ParseTuple(args, "s#", &data, &len)) {
        return NULL;
    }

    // 检查客户端是否存在
    if (!ws_obj->ws_client || !(ws_obj->ws_client)) {
        PyErr_SetString(PyExc_RuntimeError, "WebSocket client not found");
        return NULL;
    }

    // 发送数据
    bool result = (ws_obj->ws_client)->sendData(std::string(data, len));
    return PyBool_FromLong(result ? 1 : 0);
}

// close方法（适配self存储的客户端）
static PyObject* WebSocket_close(PyObject* self, PyObject* args) {
    WebSocketObject* ws_obj = (WebSocketObject*)self;

    // 解析空参数
    if (!PyArg_ParseTuple(args, "")) {
        return NULL;
    }

    // 关闭连接
    if (ws_obj->ws_client && (ws_obj->ws_client)) {
        (ws_obj->ws_client)->disconnect();
    }

    Py_RETURN_NONE;
}

// 获取event_id属性
static PyObject* WebSocket_get_id(PyObject* self, void* closure) {
    WebSocketObject* ws_obj = (WebSocketObject*)self;
    return PyInt_FromLong(ws_obj->id);
}

// 模块级函数：get_websocket
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

// 模块级函数：delete（适配self存储的客户端）
static PyObject* delete_websocket(PyObject* self, PyObject* args) {
    PyObject* obj = NULL;

    if (!PyArg_ParseTuple(args, "O", &obj)) {
        return NULL;
    }

    // 检查类型
    if (!PyObject_TypeCheck(obj, &WebSocketType)) {
        PyErr_SetString(PyExc_TypeError, "Expected WebSocket object");
        return NULL;
    }

    WebSocketObject* ws_obj = (WebSocketObject*)obj;

    // 释放客户端实例
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

// 模块方法表
static PyMethodDef ModuleMethods[] = {
    {"get_websocket", get_websocket, METH_VARARGS, "Create WebSocket"},
    {"delete", delete_websocket, METH_VARARGS, "Delete WebSocket"},
    {NULL, NULL, 0, NULL}
};

// 模块初始化
void initwebsocket(void) {
    // 确保类型已准备好
    if (PyType_Ready(&WebSocketType) < 0) {
        return;
    }

    // 创建模块
    PyObject* module = Py_InitModule3("_websocket", ModuleMethods, "WebSocket module");
    if (!module) {
        return;
    }

    // 添加WebSocket类型到模块
    Py_INCREF(&WebSocketType);
    PyModule_AddObject(module, "WebSocket", (PyObject*)&WebSocketType);
}