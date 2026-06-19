#include "mod_log.h"
#include "StartupParams.h"
#include "Logger.h"
PyObject* log(PyObject* self, PyObject* args) {
    int number;
    char* text;

    // 解析参数 - Python 2.7中字符串处理相同
    if (!PyArg_ParseTuple(args, "is", &number, &text)) {
        return NULL;
    }

    // 使用参数

    Logger::getInstance().log(number, std::string() + "[Python]" + text);
    Py_RETURN_NONE;
}

static PyMethodDef EngineMethods[] = {
    {"log", log, METH_VARARGS, "output log info"},
    {NULL, NULL, 0, NULL} // 结束标记
};

// 模块初始化函数
void initmod_log(void) {
    (void)Py_InitModule("mod_log", EngineMethods);
}