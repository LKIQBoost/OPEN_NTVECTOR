// windows.h must come first in this TU so the Windows SDK headers parse before
// any `using namespace std;` (from project headers) can make std::byte collide
// with the SDK's `byte` typedef.
#include <windows.h>
#include "PythonRuntime.h"
#include "PythonApi.h"
#include "fopmodule.h"
#include "TanGame.h"
#include "utility.h"
#include "setting.h"
#include "mod_log.h"
#include "engine.h"
#include "pkt_module.h"
#include "Logger.h"

#include "MPayWrapper.h"
extern "C" PyObject* PyInit_easy_utils(void);

// Point PYTHONHOME at the bundled python312/ directory next to Program.exe
// (CommunityBot-style deployment). Without this the embedded interpreter
// cannot find its stdlib once the exe is copied elsewhere.
static void SetPythonHome()
{
#ifdef _WIN32
    char exe[MAX_PATH] = { 0 };
    DWORD n = GetModuleFileNameA(NULL, exe, MAX_PATH);
    // Full stdlib runtime is deployed in python312/ next to the exe
    // (python312/Lib + python312/DLLs + python312/Lib/site-packages).
    // python312.dll stays in the exe dir (import-lib loading).
    std::string home = "python312";
    if (n > 0) {
        std::string p(exe);
        size_t s = p.find_last_of("\\/");
        if (s != std::string::npos) home = p.substr(0, s) + "\\python312";
    }
    std::wstring whome;
    int wlen = MultiByteToWideChar(CP_ACP, 0, home.c_str(), -1, NULL, 0);
    if (wlen > 0) {
        whome.resize(wlen);
        MultiByteToWideChar(CP_ACP, 0, home.c_str(), -1, &whome[0], wlen);
        Py_SetPythonHome(whome.c_str());
    }
#else
    Py_SetPythonHome(L".");
#endif
}

// Register a freshly created Py3 module into sys.modules so `import name` works.
static void RegisterModule(PyObject* m, const char* name)
{
    if (m) {
        if (PyDict_SetItemString(PyImport_GetModuleDict(), name, m) < 0) {
            PyErr_Print();
        }
        Py_DECREF(m);
    }
    else {
        LOG(LOG_ERROR, "[PythonRuntime] initModules failed: ", name);
        PyErr_Print();
    }
}

std::string PythonRuntime::m_script_path;
std::map<std::string, MCPFileSystem> PythonRuntime::mod_file_system_map;
MCPFileSystem PythonRuntime::m_file_system;
std::vector<uint8_t> PythonRuntime::m_last_file;
std::string PythonRuntime::m_last_filename;
//std::vector<MCPFileSystem>  PythonRuntime::McpList;
int PythonRuntime::getGlobal(
	const char* module_name,
	const char* var_name,
	PyObject** result)
{
	LOG(LOG_SCRIPTING, "[PythonRuntime] getGlobal - Fetching '", var_name, "' from module '", module_name, "'");
	PyGILState_STATE oldstate = PyGILState_Ensure();

	// ����ָ��ģ��
	PyObject* module = PyImport_ImportModule(module_name);
	if (!module || PyErr_Occurred()) {
		LOG(LOG_ERROR, "[PythonRuntime] getGlobal - Failed to load module '", module_name, "'");
		fprintf(stderr, "Can't load \"%s\"\n", module_name);
		PyGILState_Release(oldstate);
		return -1;
	}

	// ��ȡģ���еı���
	PyObject* attr = PyObject_GetAttrString(module, var_name);
	Py_DECREF(module);

	if (!attr || PyErr_Occurred()) {
		LOG(LOG_ERROR, "[PythonRuntime] getGlobal - Failed to get attribute '", var_name, "'");
		PyGILState_Release(oldstate);
		return -1;
	}

	// ת�����������
	int ret = PythonRuntime::convertResult(attr, "O", result);
	LOG(LOG_SCRIPTING, "[PythonRuntime] getGlobal - Successfully retrieved '", var_name, "'");
	PyGILState_Release(oldstate);
	return ret;
}
int PythonRuntime::getGlobalNoGIL(
	const char* module_name,
	const char* var_name,
	PyObject** result)
{

	// ����ָ��ģ��
	PyObject* module = PyImport_ImportModule(module_name);
	if (!module || PyErr_Occurred()) {
		fprintf(stderr, "Can't load \"%s\"\n", module_name);
		return -1;
	}

	// ��ȡģ���еı���
	PyObject* attr = PyObject_GetAttrString(module, var_name);
	Py_DECREF(module);

	if (!attr || PyErr_Occurred()) {
		return -1;
	}

	// ת�����������
	int ret = PythonRuntime::convertResultNoGIL(attr, "O", result);
	return ret;
}
std::vector<uint8_t> PythonRuntime::openFile(const std::string& fullname)
{
	LOG(LOG_FILE, "[PythonRuntime] openFile - Opening: ", fullname);
	// ��黺�棺����ϴδ򿪵��ļ��뱾����ͬ��ֱ�ӷ��ػ�����ļ�
	if (!m_last_file.empty() && m_last_filename == fullname) {
		LOG(LOG_FILE, "[PythonRuntime] openFile - Returning cached file, size: ", m_last_file.size());
		return m_last_file;
	}

	// ���������ļ�ϵͳ�в���
	std::vector<uint8_t> file;

	// ����ļ����Խű�·����ͷ��ȥ��·��ǰ׺
	const char* relative_path = fullname.c_str();
	if (fullname.find(m_script_path) == 0) {
		relative_path = fullname.c_str() + m_script_path.length();
	}

	// �����ļ�ϵͳ�д��ļ�

	file = m_file_system.Open(relative_path);
	if (!file.empty()) {
		LOG(LOG_FILE, "[PythonRuntime] openFile - Found in main file system, size: ", file.size());
		// ���»���
		m_last_filename = fullname;
		m_last_file = file;
		return file;
	}


	// ��������ļ�ϵͳ��û�ҵ�����ģ���ļ�ϵͳӳ���в���
	auto& mod_systems = mod_file_system_map;
	for (auto& pair : mod_systems) {
		const std::string& prefix = pair.first;
		MCPFileSystem file_system = pair.second;

		// ����ļ�����ģ��ǰ׺��ͷ��ȥ��ǰ׺
		const char* mod_relative_path = fullname.c_str();
		if (fullname.find(prefix) == 0) {
			mod_relative_path = fullname.c_str() + prefix.length();
		}

		// ��ģ���ļ�ϵͳ�д��ļ�
		file = file_system.Open(mod_relative_path);
		if (!file.empty()) {
			// ���»���
			m_last_filename = fullname;
			m_last_file = file;
			return file;
		}
	}

	return std::vector<uint8_t>();  // �������ļ�ϵͳ�ж�û�ҵ�
}
int PythonRuntime::convertResult(PyObject* py_res, const char* result_format, void* result)
{
	PyGILState_STATE oldstate = PyGILState_Ensure();

	// ���Python�����Ƿ���Ч
	if (!py_res || PyErr_Occurred()) {
		PyGILState_Release(oldstate);
		return -4;  // Python������Ч������쳣
	}

	// �������Ҫ�洢�����ֱ���ͷŶ��󲢷���
	if (!result) {
		Py_DECREF(py_res);
		PyGILState_Release(oldstate);
		return 0;
	}

	// ����Python����C++����
	int parse_success = PyArg_Parse(py_res, result_format, result);

	if (PyErr_Occurred()) {
		// ���������г���Python�쳣
		Py_DECREF(py_res);
		PyGILState_Release(oldstate);
		return -5;  // ����ʧ��
	}

	// ����Ƿ���Ҫ�ͷ�Python����
	// ����ĳЩ��ʽ����"O"�������ǲ���Ҫ�ͷŶ�����Ϊ����������
	if (result_format[0] != 'O' || result_format[1] != '\0') {
		// ���ڷǶ������ø�ʽ����Ҫ�ͷ�Python����
		Py_DECREF(py_res);
	}


	PyGILState_Release(oldstate);
	return 0;  // �ɹ�
}
int PythonRuntime::convertResultNoGIL(PyObject* py_res, const char* result_format, void* result)
{

	// ���Python�����Ƿ���Ч
	if (!py_res || PyErr_Occurred()) {
		return -4;  // Python������Ч������쳣
	}

	// �������Ҫ�洢�����ֱ���ͷŶ��󲢷���
	if (!result) {
		Py_DECREF(py_res);
		return 0;
	}

	// ����Python����C++����
	int parse_success = PyArg_Parse(py_res, result_format, result);

	if (PyErr_Occurred()) {
		// ���������г���Python�쳣
		Py_DECREF(py_res);
		return -5;  // ����ʧ��
	}

	// ����Ƿ���Ҫ�ͷ�Python����
	// ����ĳЩ��ʽ����"O"�������ǲ���Ҫ�ͷŶ�����Ϊ����������
	if (result_format[0] != 'O' || result_format[1] != '\0') {
		// ���ڷǶ������ø�ʽ����Ҫ�ͷ�Python����
		Py_DECREF(py_res);
	}


	return 0;  // �ɹ�
}
int PythonRuntime::runMethod(
	PyObject* object,
	const char* method_name,
	const char* result_format,
	void* result,
	const char* args_format,
	...)
{
	PyGILState_STATE oldstate = PyGILState_Ensure();

	// ��ȡ��������
	PyObject* method = PyObject_GetAttrString(object, method_name);
	if (!method || PyErr_Occurred()) {
		fprintf(stderr, "Can't get method \"%s\"\n", method_name);
		PyGILState_Release(oldstate);
		return -1;
	}

	// ��������
	va_list args;
	va_start(args, args_format);
	PyObject* py_args = Py_VaBuildValue(args_format, args);
	va_end(args);

	if (!py_args || PyErr_Occurred()) {
		Py_DECREF(method);
		PyGILState_Release(oldstate);
		return -1;
	}

	// ���÷���
	PyObject* py_result = PyObject_CallObject(method, py_args);

	// ���������ͷ�������
	Py_DECREF(py_args);
	Py_DECREF(method);

	if (!py_result || PyErr_Occurred()) {
		PyGILState_Release(oldstate);
		return -1;
	}

	// ת�����
	int ret = PythonRuntime::convertResult(py_result, result_format, result);
	PyGILState_Release(oldstate);
	return ret;
}
int PythonRuntime::runMethodNoGIL(
	PyObject* object,
	const char* method_name,
	const char* result_format,
	void* result,
	const char* args_format,
	...)
{

	// ��ȡ��������
	PyObject* method = PyObject_GetAttrString(object, method_name);
	if (!method || PyErr_Occurred()) {
		fprintf(stderr, "Can't get method \"%s\"\n", method_name);
		return -1;
	}

	// ��������
	va_list args;
	va_start(args, args_format);
	PyObject* py_args = Py_VaBuildValue(args_format, args);
	va_end(args);

	if (!py_args || PyErr_Occurred()) {
		Py_DECREF(method);
		return -1;
	}

	// ���÷���
	PyObject* py_result = PyObject_CallObject(method, py_args);

	// ���������ͷ�������
	Py_DECREF(py_args);
	Py_DECREF(method);

	if (!py_result || PyErr_Occurred()) {
		return -1;
	}

	// ת�����
	int ret = PythonRuntime::convertResultNoGIL(py_result, result_format, result);
	return ret;
}
void PythonRuntime::startUp()
{
	LOG(LOG_SCRIPTING, "[PythonRuntime] startUp - Beginning Python initialization");
	Py_NoSiteFlag = 1;
	Py_IgnoreEnvironmentFlag = 1;
	char src[8];
	memset(src, '\0', sizeof(src));
	Logger::getInstance().log(LOG_INFO, "Python Initialize.");
	LOG(LOG_SCRIPTING, "[PythonRuntime] startUp - Calling Py_Initialize()");
	SetPythonHome();
	Py_Initialize();
	// Py3: the GIL is auto-initialized; release it so worker threads can acquire it.
	PyEval_SaveThread();
	PyGILState_STATE state = PyGILState_Ensure();
	LOG(LOG_SCRIPTING, "[PythonRuntime] startUp - Initializing built-in modules");
	initModules();

	PyRun_SimpleStringFlags("import sys", nullptr);
	LOG(LOG_SCRIPTING, "[PythonRuntime] startUp - sys module imported");

	if (ConfigLoader::use_mcp) {
		LOG(LOG_SCRIPTING, "[PythonRuntime] startUp - Using MCP file system mode");
		if (Params::logger) {
			puts((string("[python] Use mcp at \"") + VANILLA_MCP + '\"').data());
			puts((string("[python] Use mcp init at \"") + VANILLA_MCP + '\"').data());
		}
		if (Params::logger) {
			PyRun_SimpleStringFlags("print('Python', sys.version_info)", nullptr);
			PyRun_SimpleStringFlags("print(sys.path)", nullptr);
		}


		// �����ű�·��
		std::string script_path = "vanilla.mcp";
		m_script_path = script_path;

		// ��ȡsys.path�б�
		PyObject* sys_path = nullptr;
		PythonRuntime::getGlobal("sys", "path", &sys_path);
		if (Params::logger)
			PyRun_SimpleStringFlags("print(sys.path)", nullptr);

		if (sys_path && PyList_Check(sys_path)) {
			// �������·��
			PyList_SetSlice(sys_path, 0, PyList_Size(sys_path), nullptr);

			// ���Ӹ���ģ��·��
			PythonRuntime::runMethod(sys_path, "append", src, 0, "(s)", (void*)m_script_path.c_str());

			std::string minecraft_path = m_script_path + "minecraft/";
			PythonRuntime::runMethod(sys_path, "append", src, 0, "(s)", (void*)minecraft_path.c_str());

			std::string framework_path = m_script_path + "framework/";
			PythonRuntime::runMethod(sys_path, "append", src, 0, "(s)", (void*)framework_path.c_str());

			std::string lib_path = m_script_path + "lib/";
			PythonRuntime::runMethod(sys_path, "append", src, 0, "(s)", (void*)lib_path.c_str());

			std::string lobby_path = m_script_path + "lobby/";
			PythonRuntime::runMethod(sys_path, "append", src, 0, "(s)", (void*)lobby_path.c_str());

			std::string mod_path = m_script_path + "mod/";
			PythonRuntime::runMethod(sys_path, "append", src, 0, "(s)", (void*)mod_path.c_str());

			std::string sunshine_path = m_script_path + "sunshine/";
			PythonRuntime::runMethod(sys_path, "append", src, 0, "(s)", (void*)sunshine_path.c_str());
		}
		if (Params::logger)
			PyRun_SimpleStringFlags("print(sys.path)", nullptr);
		LOG(LOG_FILE, "[PythonRuntime] startUp - Loading MCP file: ", VANILLA_MCP);
		MCPFileSystem PublicMCP(VANILLA_MCP);
		//PythonRuntime::McpList.push_back(PublicMCP);
		m_file_system = PublicMCP;
		LOG(LOG_FILE, "[PythonRuntime] startUp - Opening redirect.mcs");
		std::vector<uint8_t> file = PublicMCP.Open("redirect.mcs");
		if (file.size()) {
			LOG(LOG_SCRIPTING, "[PythonRuntime] startUp - redirect.mcs loaded, size: ", file.size());
			*(int*)file.data() = *(int*)file.data() ^ 1966019809;
			ZlibCompress zlibl;
			file = zlibl.MCPDecompress(file);
			LOG(LOG_SCRIPTING, "[PythonRuntime] startUp - Decompressed size: ", file.size());
			PyCodeObject* code_t = PythonUtils::ReadMarshalCodeObject((char*)file.data(), file.size());
			LOG(LOG_SCRIPTING, "[PythonRuntime] startUp - Executing redirect module");
			PyImport_ExecCodeModuleEx((char*)"redirect", (PyObject*)code_t, (char*)"");
			LOG(LOG_SCRIPTING, "[PythonRuntime] startUp - Importing init module");
			PyRun_SimpleString("import init");
			LOG(LOG_SCRIPTING, "[PythonRuntime] startUp - Init module imported successfully");
			//PyImport_ImportModule("init");
		}
		else
		{
			LOG(LOG_ERROR, "[PythonRuntime] startUp - redirect.mcs not found!");
			Logger::getInstance().log(LOG_ERROR, "The file does not exist.");
		}
	}
	else
	{
		LOG(LOG_SCRIPTING, "[PythonRuntime] startUp - Using Python source mode");
		if (Params::logger) {
			puts("[python] Use python source at \"source\"");
			PyRun_SimpleStringFlags("print('Python', sys.version_info)", nullptr);
			PyRun_SimpleStringFlags("print(sys.path)", nullptr);
		}
		// �����ű�·��
		std::string script_path = "./source";
		m_script_path = script_path;

		// ��ȡsys.path�б�
		PyObject* sys_path = nullptr;
		PythonRuntime::getGlobal("sys", "path", &sys_path);
		if (Params::logger)
			PyRun_SimpleStringFlags("print(sys.path)", nullptr);

		if (sys_path && PyList_Check(sys_path)) {
			// NOTE: do NOT clear sys.path here in Py3 - it would erase the
			// stdlib search paths and break every standard-library import.
			// Just append our framework/plugin paths below.

			// ���Ӹ���ģ��·��
			PythonRuntime::runMethod(sys_path, "append", src, 0, "(s)", (void*)m_script_path.c_str());

			std::string scripts_path = "./scripts";
			PythonRuntime::runMethod(sys_path, "append", src, 0, "(s)", (void*)scripts_path.c_str());

			// site-packages so third-party libraries (requests, etc.) are importable.
			std::string site_path = "./python312/Lib/site-packages";
			PythonRuntime::runMethod(sys_path, "append", src, 0, "(s)", (void*)site_path.c_str());
		}
		if (Params::logger)
			PyRun_SimpleStringFlags("print(sys.path)", nullptr);
		//PyRun_SimpleString((StringSplitUtils::escape_backslashes("sys.path.append('./source')")).data());
		PyRun_SimpleString("import init");
		//PyImport_ImportModule("init");
	}

	PyGILState_Release(state);
	//PyThreadState* main_state = PyEval_SaveThread();
}

void PythonRuntime::initModules()
{
    // Python 3: each PyInit_X() creates the module; RegisterModule puts it
    // into sys.modules so `import name` works. socket/select/ctypes are no
    // longer registered here - Python 3.12 provides them from its stdlib.
    LOG(LOG_SCRIPTING, "[PythonRuntime] initModules - Initializing api_errors module");
    RegisterModule(PyInit_api_errors(), "api_errors");
    LOG(LOG_SCRIPTING, "[PythonRuntime] initModules - Initializing nbt module");
    RegisterModule(PyInit_nbt(), "nbt");
    LOG(LOG_SCRIPTING, "[PythonRuntime] initModules - Initializing packets module");
    RegisterModule(PyInit_packets(), "packets");
    LOG(LOG_SCRIPTING, "[PythonRuntime] initModules - Initializing game_state module");
    RegisterModule(PyInit_game_state(), "game_state");
    LOG(LOG_SCRIPTING, "[PythonRuntime] initModules - Initializing client_instance module");
    RegisterModule(PyInit_client_instance(), "client_instance");
    LOG(LOG_SCRIPTING, "[PythonRuntime] initModules - Initializing easy_utils module");
    RegisterModule(PyInit_easy_utils(), "easy_utils");
    LOG(LOG_SCRIPTING, "[PythonRuntime] initModules - Initializing _client module");
    RegisterModule(PyInit__client(), "_client");
    LOG(LOG_SCRIPTING, "[PythonRuntime] initModules - Initializing tan_game module");
    register_tan_lobby_game_module();
	LOG(LOG_SCRIPTING, "[PythonRuntime] initModules - Initializing websocket module");
	RegisterModule(PyInit__websocket(), "_websocket");
	LOG(LOG_SCRIPTING, "[PythonRuntime] initModules - Initializing aes module");
	RegisterModule(PyInit_aes(), "aes");
	LOG(LOG_SCRIPTING, "[PythonRuntime] initModules - Initializing chacha module");
	RegisterModule(PyInit__chacha(), "_chacha");
	LOG(LOG_SCRIPTING, "[PythonRuntime] initModules - Initializing raknet module");
	RegisterModule(PyInit__raknet(), "_raknet");
	LOG(LOG_SCRIPTING, "[PythonRuntime] initModules - Initializing utility module");
	RegisterModule(PyInit_utility(), "utility");
	LOG(LOG_SCRIPTING, "[PythonRuntime] initModules - Initializing setting module");
	RegisterModule(PyInit_setting(), "setting");
	LOG(LOG_SCRIPTING, "[PythonRuntime] initModules - Initializing mod_log module");
	RegisterModule(PyInit_mod_log(), "mod_log");
	LOG(LOG_SCRIPTING, "[PythonRuntime] initModules - Initializing engine module");
	RegisterModule(PyInit_engine(), "engine");
	LOG(LOG_SCRIPTING, "[PythonRuntime] initModules - Initializing rotor module");
	RegisterModule(PyInit_rotor(), "rotor");
	LOG(LOG_SCRIPTING, "[PythonRuntime] initModules - Initializing fop module");
	RegisterModule(PyInit_fop(), "fop");
	LOG(LOG_SCRIPTING, "[PythonRuntime] initModules - Initializing pkt module");
	RegisterModule(PyInit_pkt(), "pkt");
	LOG(LOG_SCRIPTING, "[PythonRuntime] initModules - All modules initialized");
}
