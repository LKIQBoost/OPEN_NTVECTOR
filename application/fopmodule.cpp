#include <Python.h>
#include "Logger.h"
#include "MCPFileSystem.h"
#include "PythonRuntime.h"

static PyObject* find_file(PyObject* self, PyObject* args) {
	char* fullname;
	int length;
	char* path;
	int length_;
	if (!PyArg_ParseTuple(args, "s#s#", &fullname, &length, &path, &length_))
		return NULL;
	std::string fm(fullname, length);
	std::string pt(path, length_);
	/*
	bool find = false;
	for (size_t i = 0; i < PythonRuntime::McpList.size(); i++)
	{
		std::vector<uint8_t> mcp = PythonRuntime::McpList[i].Open(fm.data());
		if (mcp.size()) {
			find = true;
			break;
		}
	}*/

	std::string full_path;

	if (pt.data() && pt.data()[0] != '\0') {
		// 如果提供了路径，将路径和文件名组合
		full_path = pt;

		// 确保路径以分隔符结尾
		

		full_path += fm;
	}
	else {
		// 如果没有提供路径，直接使用文件名
		full_path = fm;
	}
	Logger::getInstance().log(LOG_INFO, "[FOP] find_file: " + pt + " filename: " + fm);
	return PyBool_FromLong(!PythonRuntime::openFile(full_path).empty());
}
static PyObject* get_file(PyObject* self, PyObject* args) {
	char* fullname;
	int length;
	char* path;
	int length_;
	if (!PyArg_ParseTuple(args, "s#s#", &fullname, &length, &path, &length_))
		return NULL;
	std::string fm(fullname, length);
	std::string pt(path, length_);
	Logger::getInstance().log(LOG_INFO, "[FOP] get_file: " + pt + " filename: " + fm);
	/*
	bool find = false;
	std::vector<uint8_t> mcp;
	for (size_t i = 0; i < PythonRuntime::McpList.size(); i++)
	{
		mcp = PythonRuntime::McpList[i].Open(fm.data());
		if (mcp.size()) {
			find = true;
			break;
		}
	}*/

	std::string full_path;

	if (pt.data() && pt.data()[0] != '\0') {
		// 如果提供了路径，将路径和文件名组合

		full_path = pt;
		// 确保路径以分隔符结尾
		//if (full_path.back() != '/' && full_path.back() != '\\') {
		//	full_path += '/';
		//}

		full_path += fm;
	}
	else {
		// 如果没有提供路径，直接使用文件名
		full_path = fm;
	}
	Logger::getInstance().log(LOG_INFO, "[FOP] find_file: " + pt + " filename: " + fm);
	std::vector<uint8_t> mcp = PythonRuntime::openFile(full_path);
	if (mcp.empty())
		return Py_None;
	else
		return PyString_FromStringAndSize((char*)mcp.data(), mcp.size());
}
static PyObject* fop_new_module(const char* name, PyCodeObject* code, PyObject* path) {
	PyObject* module = PyImport_ExecCodeModuleExPath((char*)name, (PyObject*)code, NULL, path);

	if (!module) {
		return NULL;
	}
	return module;
}
static PyObject* fop_new_module1(const char* name, PyCodeObject* code, PyObject* path)
{
    PyObject* sys_modules = NULL;
    PyObject* module = NULL;
    PyObject* dict = NULL;
    PyObject* result = NULL;

    // 1. 先获取 sys.modules
    sys_modules = PyImport_GetModuleDict();
    if (!sys_modules) {
        Py_INCREF(Py_None);
        return Py_None;
    }

    // 2. 检查模块是否已存在
    module = PyDict_GetItemString(sys_modules, name);
    if (module) {
        // 模块已存在，直接返回
        Py_INCREF(module);
        return module;
    }

    // 3. 创建新模块（不通过 PyImport_AddModule，我们自己添加）
    module = PyModule_New(name);
    if (!module) {
        Py_INCREF(Py_None);
        return Py_None;
    }

    // 4. 添加到 sys.modules（关键：在执行代码前添加）
    if (PyDict_SetItemString(sys_modules, name, module) < 0) {
        Py_DECREF(module);
        Py_INCREF(Py_None);
        return Py_None;
    }

    // 5. 获取模块字典
    dict = PyModule_GetDict(module);

    // 6. 设置 __builtins__
    PyObject* builtins = PyEval_GetBuiltins();
    if (builtins) {
        PyDict_SetItemString(dict, "__builtins__", builtins);
    }

    // 7. 设置包路径
    if (path && path != Py_None) {
        if (PyDict_SetItemString(dict, "__path__", path) < 0) {
            // 清理：从 sys.modules 中移除模块
            if (sys_modules) {
                PyDict_DelItemString(sys_modules, name);
            }
            Py_XDECREF(module);
            Py_INCREF(Py_None);
            return Py_None;
        }
    }

    // 8. 设置 __file__
    if (PyDict_SetItemString(dict, "__file__", code->co_filename) < 0) {
        // 清理：从 sys.modules 中移除模块
        if (sys_modules) {
            PyDict_DelItemString(sys_modules, name);
        }
        Py_XDECREF(module);
        Py_INCREF(Py_None);
        return Py_None;
    }

    // 9. 设置 __name__
    PyObject* name_obj = PyString_FromString(name);
    if (name_obj) {
        PyDict_SetItemString(dict, "__name__", name_obj);
        Py_DECREF(name_obj);
    }

    // 10. 执行代码 - 模仿 PyImport_ExecCodeModuleEx
    // 注意：这里使用 PyEval_EvalCode，但模块已经在 sys.modules 中
    result = PyEval_EvalCode((PyCodeObject*)code, dict, dict);
    if (!result) {
        // 清理：从 sys.modules 中移除模块
        if (sys_modules) {
            PyDict_DelItemString(sys_modules, name);
        }
        Py_XDECREF(module);
        Py_INCREF(Py_None);
        return Py_None;
    }
    Py_DECREF(result);

    // 11. 重新获取模块（可能被替换）
    module = PyDict_GetItemString(sys_modules, name);
    if (!module) {
        PyErr_Format(PyExc_ImportError,
            "Loaded module %s not found in sys.modules", name);
        // 清理：从 sys.modules 中移除模块
        if (sys_modules) {
            PyDict_DelItemString(sys_modules, name);
        }
        Py_XDECREF(module);
        Py_INCREF(Py_None);
        return Py_None;
    }

    Py_INCREF(module);
    return module;
}
static PyObject* new_module(PyObject* self, PyObject* args)
{
	char* name;
	PyObject* code_obj;
	PyObject* path_obj = Py_None;

	if (!PyArg_ParseTuple(args, "sO|O", &name, &code_obj, &path_obj)) {
		return NULL;
	}

	if (!PyCode_Check(code_obj)) {
		PyErr_SetString(PyExc_TypeError, "Second argument must be a code object");
		return NULL;
	}

	PyCodeObject* code = (PyCodeObject*)code_obj;
	return fop_new_module(name, code, path_obj);
}

static PyObject* new_mcp(PyObject* self, PyObject* args) {
	char* fullname;
	int length;
	const char* path = "";
	if (!PyArg_ParseTuple(args, "s#", &fullname, &length))
		return NULL;
	std::string mcp(fullname, length);

	char src[8];
	MCPFileSystem PublicMCP(mcp.c_str());
	PythonRuntime::mod_file_system_map.insert({ mcp.c_str(), PublicMCP });
	std::string module = mcp;
	PyObject* sys_path = nullptr;
	PythonRuntime::getGlobalNoGIL("sys", "path", &sys_path);
	PythonRuntime::runMethodNoGIL(sys_path, "append", src, 0, "(s)", (void*)module.c_str());
	std::vector<uint8_t> file = PublicMCP.Open("redirect.mcs");
	if (file.size()) {
		*(int*)file.data() = *(int*)file.data() ^ 1966019809;
		ZlibCompress zlibl;
		file = zlibl.MCPDecompress(file);
		PyCodeObject* code_t = PythonUtils::ReadMarshalCodeObject((char*)file.data(), file.size());
		PyImport_ExecCodeModuleEx((char*)"redirect", (PyObject*)code_t, (char*)module.c_str());
		//PyImport_ImportModule(StringSplitUtils::ToFileName(module).c_str());
		//PyRun_SimpleString("import ModMain");
		return Py_None;
	}
	else
	{
		Logger::getInstance().log(LOG_ERROR, "The file does not exist.");
		return Py_None;
	}
}
static PyObject* reload_mcp(PyObject* self, PyObject* args) {
	char* fullname;
	int length;
	if (!PyArg_ParseTuple(args, "s#", &fullname, &length))
		return NULL;

	std::string mcp(fullname, length);

	// 1. 检查是否已经加载过这个MCP
	auto it = PythonRuntime::mod_file_system_map.find(mcp.c_str());
	if (it == PythonRuntime::mod_file_system_map.end()) {
		Logger::getInstance().log(LOG_WARN, std::string() + "[MCP] MCP file not found in map: " + mcp);
		Py_RETURN_FALSE;
	}

	// 2. 移除旧的 MCPFileSystem
	PythonRuntime::mod_file_system_map.erase(it);

	// 3. 重新创建 MCPFileSystem
	MCPFileSystem PublicMCP(mcp.c_str());
	PythonRuntime::mod_file_system_map.insert({ mcp.c_str(), PublicMCP });

	Logger::getInstance().log(LOG_INFO, std::string() + "[MCP] MCP file system reloaded: " + mcp);
	//PythonRuntime::initModules();
	Py_RETURN_TRUE;
}
static struct PyMethodDef
fop_methods[] = {
	{"find_file",  find_file, METH_VARARGS},
    {"get_file",  get_file, METH_VARARGS},
	{"new_module",  new_module, METH_VARARGS},
	{"new_mcp",  new_mcp, METH_VARARGS},
	{"reload_mcp",  reload_mcp, METH_VARARGS},
    //{"load_mcp",  NULL, METH_VARARGS},
    {NULL,        NULL}		     /* sentinel */
};
//PyMODINIT_FUNC
static void
initfop(void)
{
	(void)Py_InitModule("fop", fop_methods);
	if (PyErr_Warn(PyExc_DeprecationWarning,
		"NULL") < 0)
	return;
}