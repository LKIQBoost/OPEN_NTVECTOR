#pragma once

#include <Python.h>
#include <string>

extern PyObject* ApiBotError;
extern PyObject* ApiNbtError;
extern PyObject* ApiPacketError;
extern PyObject* ApiConnectionError;
extern PyObject* ApiCryptoError;
extern PyObject* ApiClientError;
extern PyObject* ApiTimeoutError;

inline void SetApiError(PyObject* type, const std::string& message)
{
    PyErr_SetString(type ? type : PyExc_RuntimeError, message.c_str());
}
