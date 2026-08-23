#include "ErrorPython.h"
#include "PythonApi.h"

PyObject* ApiBotError = nullptr;
PyObject* ApiNbtError = nullptr;
PyObject* ApiPacketError = nullptr;
PyObject* ApiConnectionError = nullptr;
PyObject* ApiCryptoError = nullptr;
PyObject* ApiClientError = nullptr;
PyObject* ApiTimeoutError = nullptr;

namespace {

bool addException(PyObject* module, const char* shortName, const char* fullName,
                  PyObject* base, PyObject*& output)
{
    PyObject* type = PyErr_NewException(fullName, base, nullptr);
    if (!type) return false;
    Py_INCREF(type);
    output = type;
    if (PyModule_AddObject(module, shortName, type) < 0) {
        Py_DECREF(type);
        Py_CLEAR(output);
        return false;
    }
    return true;
}

PyModuleDef ErrorModule = {
    PyModuleDef_HEAD_INIT,
    "api_errors",
    "Shared exceptions for native bot APIs",
    -1,
    nullptr
};

} // namespace

extern "C" PyMODINIT_FUNC PyInit_api_errors(void)
{
    PyObject* module = PyModule_Create(&ErrorModule);
    if (!module) return nullptr;
    if (!addException(module, "BotApiError", "api_errors.BotApiError", PyExc_Exception, ApiBotError) ||
        !addException(module, "NbtError", "api_errors.NbtError", ApiBotError, ApiNbtError) ||
        !addException(module, "PacketError", "api_errors.PacketError", ApiBotError, ApiPacketError) ||
        !addException(module, "ConnectionError", "api_errors.ConnectionError", ApiBotError, ApiConnectionError) ||
        !addException(module, "CryptoError", "api_errors.CryptoError", ApiBotError, ApiCryptoError) ||
        !addException(module, "ClientError", "api_errors.ClientError", ApiBotError, ApiClientError) ||
        !addException(module, "TimeoutError", "api_errors.TimeoutError", ApiBotError, ApiTimeoutError)) {
        Py_DECREF(module);
        return nullptr;
    }
    return module;
}
