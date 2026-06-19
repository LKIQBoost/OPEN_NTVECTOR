// engine.cpp
#include <Python.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utility.h"  // 包含头文件
#include "Logger.h"
#include "EasyUtils.cpp"
PyObject* decrypt_with_tail(PyObject* self, PyObject* args) {
    char* data;
    int length;
    if (!PyArg_ParseTuple(args, "s#", &data, &length))
        return NULL;
    std::string text = Easy::decrypt_with_tail(std::string(data, length));
    return PyString_FromStringAndSize(text.data(), text.size());
}
PyObject* encrypt_with_tail(PyObject* self, PyObject* args) {
    char* data;
    int length;
    if (!PyArg_ParseTuple(args, "s#", &data, &length))
        return NULL;
    std::string text = Easy::encrypt_with_tail(std::string(data, length));
    return PyString_FromStringAndSize(text.data(), text.size());
}
PyObject* get_encrypt_token(PyObject* self, PyObject* args) {
    char* token;
    int length;
    char* url;
    char* body;
    if (!PyArg_ParseTuple(args, "s#ss", &token, &length, &body, &url))
        return NULL;
    std::string text = Easy::ComputeDynamicToken(std::string(token, length), body, url);
    return PyString_FromStringAndSize(text.data(), text.size());
}

static PyMethodDef EngineMethods[] = {
    {"decrypt_with_tail", decrypt_with_tail, METH_VARARGS, "decrypt data"},
    {"encrypt_with_tail", encrypt_with_tail, METH_VARARGS, "encrypt data"},
    {"get_encrypt_token", get_encrypt_token, METH_VARARGS, "get http dynamic token"},
    {NULL, NULL, 0, NULL} // 结束标记
};

// 模块初始化函数
void initutility(void) {
    (void)Py_InitModule("utility", EngineMethods);
}