#pragma once
#ifndef SOCKET_STATIC_H
#define SOCKET_STATIC_H

#include <Python.h>

// 核心：定义 _socket 模块的初始化函数
PyMODINIT_FUNC init_socket(void);
PyMODINIT_FUNC initselect(void);

// 定义 socket 模块的初始化函数（Python层面的socket.py的C部分）
PyMODINIT_FUNC initsocket(void);

// 可选：导出重要的 socket 函数
PyObject* _socket_socket(int family, int type, int proto);
int _socket_bind(PyObject* socket, const char* addr, int port);
int _socket_listen(PyObject* socket, int backlog);
PyObject* _socket_accept(PyObject* socket);
int _socket_connect(PyObject* socket, const char* addr, int port);
int _socket_close(PyObject* socket);


#endif // SOCKET_STATIC_H