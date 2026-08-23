
#include "PythonUtils.h"

PyCodeObject* PythonUtils::ReadMarshalCodeObject(char* code, int length)
{// ����.pyc�ļ�ͷ
    if (length < 8) {
        PyErr_SetString(PyExc_ValueError, "Invalid .pyc file: too short");
        return NULL;
    }

    const char* marshaled_data = code;
    Py_ssize_t marshaled_length = length;

    // ʹ��FILE*�����ַ����ӿ�
    // ��������ʹ���ַ����ӿڣ���Ҫģ��һ���ļ�����

    PyObject* string_obj = PyBytes_FromStringAndSize(marshaled_data, marshaled_length);
    if (!string_obj) {
        return NULL;
    }

    // ʹ��PyMarshal_ReadObjectFromString��������ã�
    // ������Python 2.7�п�����Ҫʹ����������

    // ���˵�Python����
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