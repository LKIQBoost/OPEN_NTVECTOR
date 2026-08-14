#include "setting.h"
#include "StartupParams.h"
#include "Base64Cpp.h"
#include "Py3Compat.h"
PyObject* getuid(PyObject* self, PyObject* args) {
    return PyTextFromGbk(Params::UserID);
}
PyObject* gettoken(PyObject* self, PyObject* args) {
    return PyTextFromGbk(Params::MD5Token);
}
PyObject* getplayerid(PyObject* self, PyObject* args) {
    return PyTextFromGbk(std::to_string(Params::PlayerEntityID));
}
PyObject* getengineversion(PyObject* self, PyObject* args) {
    return PyTextFromGbk(Params::EngineVersion);
}
PyObject* getpatchversion(PyObject* self, PyObject* args) {
    return PyTextFromGbk(Params::PatchVersion);
}
PyObject* getplayername(PyObject* self, PyObject* args) {
    return PyTextFromGbk(Params::DisplayName);
}

static PyMethodDef EngineMethods[] = {
    {"get_token", gettoken, METH_VARARGS, "get login token"},
    {"get_playerid", getplayerid, METH_VARARGS, "get player entity id"},
    {"get_engine_version", getengineversion, METH_VARARGS, "get engine version"},
    {"get_patch_version", getpatchversion, METH_VARARGS, "get patch version"},
    {"get_uid", getuid, METH_VARARGS, "get login uid"},
    {"get_name", getplayername, METH_VARARGS, "get player name"},
    {NULL, NULL, 0, NULL} // �������
};

// Module init (Python 3)
static struct PyModuleDef setting_module = { PyModuleDef_HEAD_INIT, "setting", NULL, -1, EngineMethods };
PyMODINIT_FUNC PyInit_setting(void) {
    return PyModule_Create(&setting_module);
}