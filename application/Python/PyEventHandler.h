#pragma once
#include <Python.h>
#include <stdarg.h>
#include <string>
#include "Logger.h"
#include "StructuredProtocol.h"

class PyEventHandler
{
public:

    PyEventHandler(PyObject* callable, bool kernel, bool structured = false) {
        handle = nullptr;
        is_kernel = kernel;
        is_structured = structured;
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
        is_structured = other.is_structured;
        Py_XINCREF(handle);
    }

    PyEventHandler& operator=(const PyEventHandler& other) {
        if (this != &other) {
            Py_XDECREF(handle);
            handle = other.handle;
            is_kernel = other.is_kernel;
            is_structured = other.is_structured;
            Py_XINCREF(handle);
        }
        return *this;
    }

    PyEventHandler(PyEventHandler&& other) noexcept {
        handle = other.handle;
        is_kernel = other.is_kernel;
        is_structured = other.is_structured;
        other.handle = nullptr;
    }

    PyEventHandler& operator=(PyEventHandler&& other) noexcept {
        if (this != &other) {
            Py_XDECREF(handle);
            handle = other.handle;
            is_kernel = other.is_kernel;
            is_structured = other.is_structured;
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
    void invokeCallback(uint32_t packetId, const std::string& data)
    {
        PyGILState_STATE state = PyGILState_Ensure();
        if (handle != nullptr) {
            PyObject* value = is_structured
                ? BuildStructuredProtocolEvent(packetId, data)
                : PyBytes_FromStringAndSize(data.data(), static_cast<Py_ssize_t>(data.size()));
            if (!value) {
                PyErr_Print();
                PyGILState_Release(state);
                return;
            }

            PyObject* args = PyTuple_New(1);
            PyTuple_SetItem(args, 0, value);
            
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
    bool is_structured;
};
