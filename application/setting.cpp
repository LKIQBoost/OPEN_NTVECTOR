#include "setting.h"
#include "StartupParams.h"
#include "Base64Cpp.h"
#include "WindowsEncodingConverter.h"
PyObject* getuid(PyObject* self, PyObject* args) {
    return PyString_FromString(Params::UserID.c_str());
}
PyObject* gettoken(PyObject* self, PyObject* args) {
    return PyString_FromStringAndSize(Params::MD5Token.c_str(), Params::MD5Token.size());
}
PyObject* getplayerid(PyObject* self, PyObject* args) {
    return PyString_FromString(std::to_string(Params::PlayerEntityID).c_str());
}
PyObject* getengineversion(PyObject* self, PyObject* args) {
    return PyString_FromString(Params::EngineVersion.c_str());
}
PyObject* getpatchversion(PyObject* self, PyObject* args) {
    return PyString_FromString(Params::PatchVersion.c_str());
}
PyObject* getplayername(PyObject* self, PyObject* args) {
    return PyString_FromString(WindowsEncodingConverter::gbkToUtf8(Params::DisplayName).c_str());
}

static PyMethodDef EngineMethods[] = {
    {"get_token", gettoken, METH_VARARGS, "get login token"},
    {"get_playerid", getplayerid, METH_VARARGS, "get player entity id"},
    {"get_engine_version", getengineversion, METH_VARARGS, "get engine version"},
    {"get_patch_version", getpatchversion, METH_VARARGS, "get patch version"},
    {"get_uid", getuid, METH_VARARGS, "get login uid"},
    {"get_name", getplayername, METH_VARARGS, "get player name"},
    {NULL, NULL, 0, NULL} // 结束标记
};

// 模块初始化函数
void initsetting(void) {
    (void)Py_InitModule("setting", EngineMethods);
}