#include <Python.h>
#include "Logger.h"
#include "MCPFileSystem.h"
#include "PythonRuntime.h"

static PyObject* find_file(PyObject* self, PyObject* args) {
	char* fullname;
	Py_ssize_t length;
	char* path;
	Py_ssize_t length_;
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
		// ����ṩ��·������·�����ļ������
		full_path = pt;

		// ȷ��·���Էָ�����β
		

		full_path += fm;
	}
	else {
		// ���û���ṩ·����ֱ��ʹ���ļ���
		full_path = fm;
	}
	Logger::getInstance().log(LOG_INFO, "[FOP] find_file: " + pt + " filename: " + fm);
	return PyBool_FromLong(!PythonRuntime::openFile(full_path).empty());
}
static PyObject* get_file(PyObject* self, PyObject* args) {
	char* fullname;
	Py_ssize_t length;
	char* path;
	Py_ssize_t length_;
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
		// ����ṩ��·������·�����ļ������

		full_path = pt;
		// ȷ��·���Էָ�����β
		//if (full_path.back() != '/' && full_path.back() != '\\') {
		//	full_path += '/';
		//}

		full_path += fm;
	}
	else {
		// ���û���ṩ·����ֱ��ʹ���ļ���
		full_path = fm;
	}
	Logger::getInstance().log(LOG_INFO, "[FOP] find_file: " + pt + " filename: " + fm);
	std::vector<uint8_t> mcp = PythonRuntime::openFile(full_path);
	if (mcp.empty())
		return Py_None;
	else
		return PyBytes_FromStringAndSize((char*)mcp.data(), (Py_ssize_t)mcp.size());
}
static PyObject* fop_new_module(const char* name, PyCodeObject* code, PyObject* path) {
	const char* pathname = NULL;
	if (path && path != Py_None) {
		pathname = PyUnicode_AsUTF8(path);
	}
	PyObject* module = PyImport_ExecCodeModuleEx((char*)name, (PyObject*)code, pathname);

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

    // 1. �Ȼ�ȡ sys.modules
    sys_modules = PyImport_GetModuleDict();
    if (!sys_modules) {
        Py_INCREF(Py_None);
        return Py_None;
    }

    // 2. ���ģ���Ƿ��Ѵ���
    module = PyDict_GetItemString(sys_modules, name);
    if (module) {
        // ģ���Ѵ��ڣ�ֱ�ӷ���
        Py_INCREF(module);
        return module;
    }

    // 3. ������ģ�飨��ͨ�� PyImport_AddModule�������Լ����ӣ�
    module = PyModule_New(name);
    if (!module) {
        Py_INCREF(Py_None);
        return Py_None;
    }

    // 4. ���ӵ� sys.modules���ؼ�����ִ�д���ǰ���ӣ�
    if (PyDict_SetItemString(sys_modules, name, module) < 0) {
        Py_DECREF(module);
        Py_INCREF(Py_None);
        return Py_None;
    }

    // 5. ��ȡģ���ֵ�
    dict = PyModule_GetDict(module);

    // 6. ���� __builtins__
    PyObject* builtins = PyEval_GetBuiltins();
    if (builtins) {
        PyDict_SetItemString(dict, "__builtins__", builtins);
    }

    // 7. ���ð�·��
    if (path && path != Py_None) {
        if (PyDict_SetItemString(dict, "__path__", path) < 0) {
            // �������� sys.modules ���Ƴ�ģ��
            if (sys_modules) {
                PyDict_DelItemString(sys_modules, name);
            }
            Py_XDECREF(module);
            Py_INCREF(Py_None);
            return Py_None;
        }
    }

    // 8. set __file__ (Py3: code->co_filename is opaque; read the attribute)
    PyObject* filename_obj = PyObject_GetAttrString((PyObject*)code, "co_filename");
    if (PyDict_SetItemString(dict, "__file__", filename_obj) < 0) {
        // �������� sys.modules ���Ƴ�ģ��
        if (sys_modules) {
            PyDict_DelItemString(sys_modules, name);
        }
        Py_XDECREF(module);
        Py_INCREF(Py_None);
        return Py_None;
    }

    // 9. set __name__
    PyObject* name_obj = PyUnicode_FromString(name);
    if (name_obj) {
        PyDict_SetItemString(dict, "__name__", name_obj);
        Py_DECREF(name_obj);
    }

    // 10. ִ�д��� - ģ�� PyImport_ExecCodeModuleEx
    // ע�⣺����ʹ�� PyEval_EvalCode����ģ���Ѿ��� sys.modules ��
    result = PyEval_EvalCode((PyObject*)code, dict, dict);
    if (!result) {
        // �������� sys.modules ���Ƴ�ģ��
        if (sys_modules) {
            PyDict_DelItemString(sys_modules, name);
        }
        Py_XDECREF(module);
        Py_INCREF(Py_None);
        return Py_None;
    }
    Py_DECREF(result);

    // 11. ���»�ȡģ�飨���ܱ��滻��
    module = PyDict_GetItemString(sys_modules, name);
    if (!module) {
        PyErr_Format(PyExc_ImportError,
            "Loaded module %s not found in sys.modules", name);
        // �������� sys.modules ���Ƴ�ģ��
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

	// 1. ����Ƿ��Ѿ����ع����MCP
	auto it = PythonRuntime::mod_file_system_map.find(mcp.c_str());
	if (it == PythonRuntime::mod_file_system_map.end()) {
		Logger::getInstance().log(LOG_WARN, std::string() + "[MCP] MCP file not found in map: " + mcp);
		Py_RETURN_FALSE;
	}

	// 2. �Ƴ��ɵ� MCPFileSystem
	PythonRuntime::mod_file_system_map.erase(it);

	// 3. ���´��� MCPFileSystem
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
// Module init (Python 3). Must stay static: fopmodule.h #includes fopmodule.cpp,
// so this file is compiled both standalone and inside PythonRuntime.cpp's TU.
static struct PyModuleDef fop_module = { PyModuleDef_HEAD_INIT, "fop", NULL, -1, fop_methods };
static PyObject*
PyInit_fop(void)
{
	return PyModule_Create(&fop_module);
}