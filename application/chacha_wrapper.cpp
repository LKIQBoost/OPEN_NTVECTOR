#include "chacha_wrapper.h"
#include <Python.h>
#include "ChaChaX.h"

#ifdef __cplusplus
extern "C" {
#endif
    extern PyTypeObject PyChaChaX_Type;
    // ChaChaX�����Python��װ�ṹ
    typedef struct {
        PyObject_HEAD
            ChaChaX* chacha;  // ʵ�ʵ�C++����ָ��
    } PyChaChaX;

    // ���ٺ�����_chacha.delete(ct)
    static PyObject* chacha_delete(PyObject* self, PyObject* args) {
        PyObject* py_obj = NULL;
        if (!PyArg_ParseTuple(args, "O", &py_obj))
            return NULL;

        // ����Ƿ���PyChaChaX����
        if (Py_TYPE(py_obj) != &PyChaChaX_Type) {
            PyErr_SetString(PyExc_TypeError, "Expected ChaChaX object");
            return NULL;
        }

        PyChaChaX* chacha_obj = (PyChaChaX*)py_obj;
        if (chacha_obj->chacha) {
            delete chacha_obj->chacha;  // ɾ��C++����
            chacha_obj->chacha = NULL;
        }

        Py_RETURN_NONE;
    }

    // ����������_chacha.get_chacha()
    static PyObject* chacha_get_chacha(PyObject* self, PyObject* args) {
        uint32_t rounds;
        PyObject* key_bytes;

        if (!PyArg_ParseTuple(args, "IO", &rounds, &key_bytes))
            return NULL;

        // ���key�Ƿ�Ϊbytes����
        if (!PyBytes_Check(key_bytes)) {
            PyErr_SetString(PyExc_TypeError, "Key must be bytes");
            return NULL;
        }

        // ���key����
        Py_ssize_t key_len = PyBytes_Size(key_bytes);
        if (key_len < 32) {
            PyErr_SetString(PyExc_ValueError, "Key must be at least 32 bytes");
            return NULL;
        }

        // ����Python����
        PyChaChaX* py_chacha = PyObject_New(PyChaChaX, &PyChaChaX_Type);
        if (!py_chacha)
            return NULL;

        // ����C++����
        uint8_t* key_data = (uint8_t*)PyBytes_AsString(key_bytes);
        py_chacha->chacha = new ChaChaX(rounds, key_data);

        return (PyObject*)py_chacha;
    }

    // ����������ct.process(data)
    static PyObject* chacha_process(PyObject* self, PyObject* args) {
        PyObject* py_obj = NULL;
        PyObject* data_bytes = NULL;

        if (!PyArg_ParseTuple(args, "OO", &py_obj, &data_bytes))
            return NULL;

        // ����������
        if (Py_TYPE(py_obj) != &PyChaChaX_Type) {
            PyErr_SetString(PyExc_TypeError, "Expected ChaChaX object");
            return NULL;
        }

        // ��������Ƿ�Ϊbytes
        if (!PyBytes_Check(data_bytes)) {
            PyErr_SetString(PyExc_TypeError, "Data must be bytes");
            return NULL;
        }

        PyChaChaX* chacha_obj = (PyChaChaX*)py_obj;
        if (!chacha_obj->chacha) {
            PyErr_SetString(PyExc_RuntimeError, "ChaChaX object already deleted");
            return NULL;
        }

        // �������ݣ�processData��ֱ���޸�
        Py_ssize_t data_len = PyBytes_Size(data_bytes);
        uint8_t* data = (uint8_t*)malloc(data_len);
        if (!data) {
            PyErr_SetString(PyExc_MemoryError, "Failed to allocate memory");
            return NULL;
        }

        memcpy(data, PyBytes_AsString(data_bytes), data_len);

        // ��������
        chacha_obj->chacha->processData(data, data_len);

        // ���ش������bytes
        PyObject* result = PyBytes_FromStringAndSize((char*)data, data_len);
        free(data);

        return result;
    }

    // ��������
    static PyMethodDef ChaChaMethods[] = {
        {"get_chacha", chacha_get_chacha, METH_VARARGS, "Create ChaChaX object"},
        {"delete", chacha_delete, METH_VARARGS, "Delete ChaChaX object"},
        {"process", chacha_process, METH_VARARGS, "Process data"},
        {NULL, NULL, 0, NULL}
    };

    // ���Ͷ���
    PyTypeObject PyChaChaX_Type = {
        PyObject_HEAD_INIT(NULL)
        0,                         // ob_size
        "_chacha.ChaChaX",         // tp_name
        sizeof(PyChaChaX),         // tp_basicsize
        0,                         // tp_itemsize
        0,                         // tp_dealloc
        0,                         // tp_print
        0,                         // tp_getattr
        0,                         // tp_setattr
        0,                         // tp_compare
        0,                         // tp_repr
        0,                         // tp_as_number
        0,                         // tp_as_sequence
        0,                         // tp_as_mapping
        0,                         // tp_hash
        0,                         // tp_call
        0,                         // tp_str
        0,                         // tp_getattro
        0,                         // tp_setattro
        0,                         // tp_as_buffer
        Py_TPFLAGS_DEFAULT,        // tp_flags
        "ChaChaX wrapper object",  // tp_doc
    };

    // ģ���ʼ������
    void init_chacha(void) {
        // ��ʼ������
        PyChaChaX_Type.tp_new = PyType_GenericNew;
        if (PyType_Ready(&PyChaChaX_Type) < 0)
            return;

        // ����ģ��
        PyObject* module = Py_InitModule("_chacha", ChaChaMethods);
        if (!module)
            return;

        // �������͵�ģ��
        Py_INCREF(&PyChaChaX_Type);
        PyModule_AddObject(module, "ChaChaX", (PyObject*)&PyChaChaX_Type);
    }

#ifdef __cplusplus
}
#endif