#pragma once
#include <Python.h>
#include <stdarg.h>
#include <string>
#include "Logger.h"

class PyEventHandler
{
public:

    PyEventHandler(PyObject* callable, bool kernel) {
        handle = nullptr;
        is_kernel = kernel;
        if (callable == nullptr) {
            Logger::getInstance().logv(LOG_ERROR, "PyEventHandler: callable is null.");
        }
        else
        {
            int code = PyCallable_Check(callable);
            if (code == 0) {
                Logger::getInstance().logv(LOG_ERROR, "PyEventHandler: callable object is invalid.");
                handle = nullptr;
            }
            else
            {
                Py_INCREF(callable);
                handle = callable;
            }
        }
    }
    PyEventHandler(const PyEventHandler& other) {
        handle = other.handle;
        is_kernel = other.is_kernel;
        Py_XINCREF(handle);
    }

    PyEventHandler& operator=(const PyEventHandler& other) {
        if (this != &other) {
            Py_XDECREF(handle);
            handle = other.handle;
            is_kernel = other.is_kernel;
            Py_XINCREF(handle);
        }
        return *this;
    }

    PyEventHandler(PyEventHandler&& other) noexcept {
        handle = other.handle;
        is_kernel = other.is_kernel;
        other.handle = nullptr;
    }

    PyEventHandler& operator=(PyEventHandler&& other) noexcept {
        if (this != &other) {
            Py_XDECREF(handle);
            handle = other.handle;
            is_kernel = other.is_kernel;
            other.handle = nullptr;
        }
        return *this;
    }
    ~PyEventHandler() {
        Py_XDECREF(handle);
    }
    bool isKernel() const {
        return is_kernel;
    }
    void invokeCallback(std::string _data)
    {
        PyGILState_STATE state = PyGILState_Ensure();
        if (handle != nullptr) {
            if (_data.empty()) {
                Logger::getInstance().logv(LOG_ERROR, "PyEventHandler: data is empty.");
                PyGILState_Release(state);
                return;
            }
            PyObject* str = PyBytes_FromStringAndSize(_data.c_str(), (Py_ssize_t)_data.size());

            PyObject* args = PyTuple_New(1);
            PyTuple_SetItem(args, 0, str);
            
            PyObject* result = PyObject_CallObject(handle, args);
            if (result == NULL) {
                PyErr_Print(); // ��ӡPython�쳣
            }
            else {
                Py_DECREF(result);
            }
            Py_XDECREF(args);
        }
        else
        {
            Logger::getInstance().logv(LOG_WARN, "PyEventHandler: handle is null, cannot invoke callback.");
        }
        PyGILState_Release(state);
    }
private:
    PyObject* handle;
    bool is_kernel;
};

