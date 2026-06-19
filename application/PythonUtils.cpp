
#include "PythonUtils.h"

PyCodeObject* PythonUtils::ReadMarshalCodeObject(char* code, int length)
{// 跳过.pyc文件头
    if (length < 8) {
        PyErr_SetString(PyExc_ValueError, "Invalid .pyc file: too short");
        return NULL;
    }

    const char* marshaled_data = code;
    Py_ssize_t marshaled_length = length;

    // 使用FILE*或者字符串接口
    // 这里我们使用字符串接口，需要模拟一个文件对象

    PyObject* string_obj = PyString_FromStringAndSize(marshaled_data, marshaled_length);
    if (!string_obj) {
        return NULL;
    }

    // 使用PyMarshal_ReadObjectFromString（如果可用）
    // 或者在Python 2.7中可能需要使用其他方法

    // 回退到Python方法
    PyObject* marshal_module = PyImport_ImportModule("marshal");
    if (!marshal_module) {
        Py_DECREF(string_obj);
        return NULL;
    }

    PyObject* loads_func = PyObject_GetAttrString(marshal_module, "loads");
    Py_DECREF(marshal_module);
    if (!loads_func) {
        Py_DECREF(string_obj);
        return NULL;
    }

    PyObject* code_obj = PyObject_CallFunctionObjArgs(loads_func, string_obj, NULL);
    Py_DECREF(loads_func);
    Py_DECREF(string_obj);

    if (!code_obj || !PyCode_Check(code_obj)) {
        if (code_obj) Py_DECREF(code_obj);
        return NULL;
    }

    return (PyCodeObject*)code_obj;
}