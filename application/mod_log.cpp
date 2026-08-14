#include "mod_log.h"
#include "StartupParams.h"
#include "Logger.h"
#include "Py3Compat.h"
PyObject* log(PyObject* self, PyObject* args) {
    int number;
    char* text;

    // �������� - Python 2.7���ַ���������ͬ
    if (!PyArg_ParseTuple(args, "is", &number, &text)) {
        return NULL;
    }

    // ʹ�ò���

    // Py3: 's' gives UTF-8; convert to GBK so the logger/console shows Chinese correctly.
    Logger::getInstance().log(number, std::string("[Python]") + Utf8ToGbk(text));
    Py_RETURN_NONE;
}

static PyMethodDef EngineMethods[] = {
    {"log", log, METH_VARARGS, "output log info"},
    {NULL, NULL, 0, NULL} // �������
};

// Module init (Python 3)
static struct PyModuleDef mod_log_module = { PyModuleDef_HEAD_INIT, "mod_log", NULL, -1, EngineMethods };
PyMODINIT_FUNC PyInit_mod_log(void) {
    return PyModule_Create(&mod_log_module);
}