#include "client_wrapper.h"
#include "LoginSession.h"
#include "LoginAuth.h"
#include "ConfigLoader.h"
#include <Python.h>
#include <stdlib.h>
#include <string.h>


// 2. ����ʵ����raknet.get_raknet()
static PyObject*
raknet_get_client(PyObject* self, PyObject* args)
{
    // ����C�ṹ���ڴ�
    ClientInstance* inst = new ClientInstance();
    if (!inst) {
        PyErr_SetString(PyExc_MemoryError, "Failed to allocate RakNet instance");
        return NULL;
    }

    // ��Cָ���װ��PyCObject���أ�Python����������װ����
    return PyCapsule_New(inst, "client", NULL);
}
/*
// 3. ��ȡ���ԣ�raknet.get(self) �� ������get_port_ipΪ��
static PyObject*
raknet_get_port_ip(PyObject* self, PyObject* args)
{
    // ����Python�㴫���PyCObject����RakNetʵ����װ����
    PyObject* py_inst;
    if (!PyArg_ParseTuple(args, "O", &py_inst)) {
        return NULL;
    }

    // ��PyCObject����ȡC��ָ��
    RakNetInstance* inst = (RakNetInstance*)PyCapsule_GetPointer(py_inst, "client");
    if (!inst) {
        PyErr_SetString(PyExc_ValueError, "Invalid RakNet instance");
        return NULL;
    }

    // ��������Ԫ�飨�˿ڡ�IP��
    return Py_BuildValue("is", inst->port, inst->server_ip);
}
*/
/*
// 4. �������ԣ�raknet.set**(self, **) �� ������set_port_ipΪ��
static PyObject*
raknet_set_port_ip(PyObject* self, PyObject* args)
{
    PyObject* py_inst;
    int port;
    char* server_ip;

    // ����������ʵ�����˿ڡ�IP
    if (!PyArg_ParseTuple(args, "Ois", &py_inst, &port, &server_ip)) {
        return NULL;
    }

    // ��ȡC��ָ��
    RakNetInstance* inst = (RakNetInstance*)PyCapsule_GetPointer(py_inst, "client");
    if (!inst) {
        PyErr_SetString(PyExc_ValueError, "Invalid RakNet instance");
        return NULL;
    }

    // �������ԣ�ע���ַ����ڴ������
    inst->port = port;
    free(inst->server_ip);          // �ͷž�IP
    inst->server_ip = strdup(server_ip);  // ������IP

    Py_RETURN_NONE;
}
*/
/*
// 5. ����ʵ����raknet.delete(self)
static PyObject*
raknet_delete(PyObject* self, PyObject* args)
{
    PyObject* py_inst;
    if (!PyArg_ParseTuple(args, "O", &py_inst)) {
        return NULL;
    }

    // ��ȡC��ָ��
    RakNetInstance* inst = (RakNetInstance*)PyCapsule_GetPointer(py_inst, "client");
    if (!inst) {
        PyErr_SetString(PyExc_ValueError, "Invalid RakNet instance");
        return NULL;
    }

    // �ͷ�C�ṹ���ڲ���Դ
    free(inst->server_ip);
    // �ͷŽṹ�屾��
    free(inst);

    // ���PyCObject��ָ�루�����ظ��ͷţ�
    PyCapsule_SetPointer(py_inst, NULL);

    Py_RETURN_NONE;
}
*/

static PyObject*
disconnect(PyObject* self, PyObject* args)
{
    PyObject* py_inst;

    // ����������ʵ�����˿ڡ�IP
    if (!PyArg_ParseTuple(args, "O", &py_inst)) {
        return NULL;
    }

    // ��ȡC��ָ��
    ClientInstance* inst = (ClientInstance*)PyCapsule_GetPointer(py_inst, "client");
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
    Py_ssize_t length;
    char* DisplayName;
    char* UserID;
    char* EngineVersion;
    char* PatchVersion;
    char* AuthServerUrl;
    char* NeteaseServerID;
    char* ServerIP;
    int port;
    if (!PyArg_ParseTuple(args, "Oy#sssssssi", &py_inst, &MD5Token, &length,
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

    ClientInstance* inst = (ClientInstance*)PyCapsule_GetPointer(py_inst, "client");
    if (!inst) {
        PyErr_SetString(PyExc_ValueError, "Invalid Client instance");
        return NULL;
    }

    // �� ������:�� Login �� startUp
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

    // ��ȡC��ָ��
    ClientInstance* inst = (ClientInstance*)PyCapsule_GetPointer(py_inst, "client");
    if (!inst) {
        PyErr_SetString(PyExc_ValueError, "Invalid RakNet instance");
        return NULL;
    }
    inst->disconnect();
    free(inst);

    // ���PyCObject��ָ�루�����ظ��ͷţ�
    PyCapsule_SetPointer(py_inst, NULL);

    Py_RETURN_NONE;
}

// 6. �����б���ӳ��Python��������C����
static PyMethodDef RakNetMethods[] = {
    {"get_client",  raknet_get_client, METH_NOARGS, "Create a new Client instance"},
    {"startUp", startUp, METH_VARARGS, ""},
    {"disconnect", disconnect, METH_VARARGS, ""},
    {"delete",      client_delete,      METH_VARARGS, "Destroy Client instance"},
    {NULL, NULL, 0, NULL}  // �������
};

// Module init (Python 3) - this whole file is dead code (never registered),
// kept only so it still compiles. The live _client lives in engine.cpp.
static struct PyModuleDef _client_module = { PyModuleDef_HEAD_INIT, "_client", NULL, -1, RakNetMethods };
static PyObject* _initclient(void)
{
    return PyModule_Create(&_client_module);
}