// engine.cpp
#include <Python.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utility.h"  // ����ͷ�ļ�
#include "Logger.h"
#include "EasyUtils.cpp"
PyObject* decrypt_with_tail(PyObject* self, PyObject* args) {
    char* data;
    Py_ssize_t length;
    if (!PyArg_ParseTuple(args, "y#", &data, &length))
        return NULL;
    std::string text = Easy::decrypt_with_tail(std::string(data, (size_t)length));
    return PyBytes_FromStringAndSize(text.data(), (Py_ssize_t)text.size());
}
PyObject* encrypt_with_tail(PyObject* self, PyObject* args) {
    char* data;
    Py_ssize_t length;
    if (!PyArg_ParseTuple(args, "y#", &data, &length))
        return NULL;
    std::string text = Easy::encrypt_with_tail(std::string(data, (size_t)length));
    return PyBytes_FromStringAndSize(text.data(), (Py_ssize_t)text.size());
}
PyObject* get_encrypt_token(PyObject* self, PyObject* args) {
    char* token;
    Py_ssize_t length;
    char* url;
    char* body;
    if (!PyArg_ParseTuple(args, "y#ss", &token, &length, &body, &url))
        return NULL;
    std::string text = Easy::ComputeDynamicToken(std::string(token, (size_t)length), body, url);
    return PyBytes_FromStringAndSize(text.data(), (Py_ssize_t)text.size());
}

static PyMethodDef EngineMethods[] = {
    {"decrypt_with_tail", decrypt_with_tail, METH_VARARGS, "decrypt data"},
    {"encrypt_with_tail", encrypt_with_tail, METH_VARARGS, "encrypt data"},
    {"get_encrypt_token", get_encrypt_token, METH_VARARGS, "get http dynamic token"},
    {NULL, NULL, 0, NULL} // �������
};

// Module init (Python 3)
static struct PyModuleDef utility_module = { PyModuleDef_HEAD_INIT, "utility", NULL, -1, EngineMethods };
PyMODINIT_FUNC PyInit_utility(void) {
    return PyModule_Create(&utility_module);
}