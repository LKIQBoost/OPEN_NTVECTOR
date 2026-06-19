#include "client_wrapper.h"
#include "LoginSession.h"
#include "LoginAuth.h"
#include "ConfigLoader.h"
#include <Python.h>
#include <stdlib.h>
#include <string.h>


// 2. 创建实例：raknet.get_raknet()
static PyObject*
raknet_get_client(PyObject* self, PyObject* args)
{
    // 分配C结构体内存
    ClientInstance* inst = new ClientInstance();
    if (!inst) {
        PyErr_SetString(PyExc_MemoryError, "Failed to allocate RakNet instance");
        return NULL;
    }

    // 将C指针包装成PyCObject返回（Python层持有这个包装对象）
    return PyCObject_FromVoidPtr(inst, NULL);
}
/*
// 3. 获取属性：raknet.get(self) → 这里以get_port_ip为例
static PyObject*
raknet_get_port_ip(PyObject* self, PyObject* args)
{
    // 解析Python层传入的PyCObject（即RakNet实例包装对象）
    PyObject* py_inst;
    if (!PyArg_ParseTuple(args, "O", &py_inst)) {
        return NULL;
    }

    // 从PyCObject中提取C层指针
    RakNetInstance* inst = (RakNetInstance*)PyCObject_AsVoidPtr(py_inst);
    if (!inst) {
        PyErr_SetString(PyExc_ValueError, "Invalid RakNet instance");
        return NULL;
    }

    // 返回属性元组（端口、IP）
    return Py_BuildValue("is", inst->port, inst->server_ip);
}
*/
/*
// 4. 设置属性：raknet.set**(self, **) → 这里以set_port_ip为例
static PyObject*
raknet_set_port_ip(PyObject* self, PyObject* args)
{
    PyObject* py_inst;
    int port;
    char* server_ip;

    // 解析参数：实例、端口、IP
    if (!PyArg_ParseTuple(args, "Ois", &py_inst, &port, &server_ip)) {
        return NULL;
    }

    // 提取C层指针
    RakNetInstance* inst = (RakNetInstance*)PyCObject_AsVoidPtr(py_inst);
    if (!inst) {
        PyErr_SetString(PyExc_ValueError, "Invalid RakNet instance");
        return NULL;
    }

    // 设置属性（注意字符串内存管理）
    inst->port = port;
    free(inst->server_ip);          // 释放旧IP
    inst->server_ip = strdup(server_ip);  // 拷贝新IP

    Py_RETURN_NONE;
}
*/
/*
// 5. 销毁实例：raknet.delete(self)
static PyObject*
raknet_delete(PyObject* self, PyObject* args)
{
    PyObject* py_inst;
    if (!PyArg_ParseTuple(args, "O", &py_inst)) {
        return NULL;
    }

    // 提取C层指针
    RakNetInstance* inst = (RakNetInstance*)PyCObject_AsVoidPtr(py_inst);
    if (!inst) {
        PyErr_SetString(PyExc_ValueError, "Invalid RakNet instance");
        return NULL;
    }

    // 释放C结构体内部资源
    free(inst->server_ip);
    // 释放结构体本身
    free(inst);

    // 清空PyCObject的指针（避免重复释放）
    PyCObject_SetVoidPtr(py_inst, NULL);

    Py_RETURN_NONE;
}
*/

static PyObject*
disconnect(PyObject* self, PyObject* args)
{
    PyObject* py_inst;

    // 解析参数：实例、端口、IP
    if (!PyArg_ParseTuple(args, "O", &py_inst)) {
        return NULL;
    }

    // 提取C层指针
    ClientInstance* inst = (ClientInstance*)PyCObject_AsVoidPtr(py_inst);
    if (!inst) {
        PyErr_SetString(PyExc_ValueError, "Invalid RakNet instance");
        return NULL;
    }
    inst->disconnect();

    Py_RETURN_NONE;
}
static PyObject*
startUp(PyObject* self, PyObject* args)
{
    PyObject* py_inst;
    char* MD5Token;
    int length;
    char* DisplayName;
    char* UserID;
    char* EngineVersion;
    char* PatchVersion;
    char* AuthServerUrl;
    char* NeteaseServerID;
    char* ServerIP;
    int port;
    if (!PyArg_ParseTuple(args, "Os#sssssssi", &py_inst, &MD5Token, &length,
        &DisplayName, &UserID, &EngineVersion, &PatchVersion,
        &AuthServerUrl, &ServerIP, &NeteaseServerID, &port)) {
        return NULL;
    }

    ChainPair Pair;
    Pair.AuthServerUrl = AuthServerUrl;
    Pair.DisplayName = DisplayName;
    Pair.EngineVersion = EngineVersion;
    Pair.PatchVersion = PatchVersion;
    Pair.MD5Token = std::string(MD5Token, 16);
    Pair.UserID = UserID;
    Pair.NeteaseServerID = NeteaseServerID;

    ClientInstance* inst = (ClientInstance*)PyCObject_AsVoidPtr(py_inst);
    if (!inst) {
        PyErr_SetString(PyExc_ValueError, "Invalid Client instance");
        return NULL;
    }

    // ★ 新流程:先 Login 再 startUp
    LoginSession session = LoginAuth::Login(Pair, ConfigLoader::SkinData);
    if (!session.valid()) {
        PyErr_SetString(PyExc_RuntimeError, "Login failed");
        return NULL;
    }
    inst->startUp(std::move(session), ServerIP, port);

    Py_RETURN_NONE;
}
static PyObject*
client_delete(PyObject* self, PyObject* args)
{
    PyObject* py_inst;
    if (!PyArg_ParseTuple(args, "O", &py_inst)) {
        return NULL;
    }

    // 提取C层指针
    ClientInstance* inst = (ClientInstance*)PyCObject_AsVoidPtr(py_inst);
    if (!inst) {
        PyErr_SetString(PyExc_ValueError, "Invalid RakNet instance");
        return NULL;
    }
    inst->disconnect();
    free(inst);

    // 清空PyCObject的指针（避免重复释放）
    PyCObject_SetVoidPtr(py_inst, NULL);

    Py_RETURN_NONE;
}

// 6. 方法列表：映射Python调用名到C函数
static PyMethodDef RakNetMethods[] = {
    {"get_client",  raknet_get_client, METH_NOARGS, "Create a new Client instance"},
    {"startUp", startUp, METH_VARARGS, ""},
    {"disconnect", disconnect, METH_VARARGS, ""},
    {"delete",      client_delete,      METH_VARARGS, "Destroy Client instance"},
    {NULL, NULL, 0, NULL}  // 结束标记
};

// 7. 模块初始化（Python 2.7 固定格式：init+模块名）
static void _initclient(void)
{
    (void)Py_InitModule("_client", RakNetMethods);
}