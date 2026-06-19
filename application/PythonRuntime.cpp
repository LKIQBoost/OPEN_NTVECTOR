#include "PythonRuntime.h"
#include "fopmodule.h"
#include "ctypes.h"
#include "TanGame.h"
#include "utility.h"
#include "setting.h"
#include "mod_log.h"
#include "engine.h"
#include "socket_static.h"
#include "Logger.h"

#include "MPayWrapper.h"
extern "C" void initeasy_utils(void);

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
	Py_Initialize();
	if (!PyEval_ThreadsInitialized()) {
		PyEval_InitThreads();  // ��ʼ���߳�֧��
		PyEval_SaveThread();        // ��ؼ�2���ͷ����̳߳��е�GIL���������߳��ܻ�ȡ������
	}
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
			PyRun_SimpleStringFlags("print 'Python', sys.version_info", nullptr);
			PyRun_SimpleStringFlags("print sys.path", nullptr);
		}


		// �����ű�·��
		std::string script_path = "vanilla.mcp";
		m_script_path = script_path;

		// ��ȡsys.path�б�
		PyObject* sys_path = nullptr;
		PythonRuntime::getGlobal("sys", "path", &sys_path);
		if (Params::logger)
			PyRun_SimpleStringFlags("print sys.path", nullptr);

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
			PyRun_SimpleStringFlags("print sys.path", nullptr);
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
			PyRun_SimpleStringFlags("print 'Python', sys.version_info", nullptr);
			PyRun_SimpleStringFlags("print sys.path", nullptr);
		}
		// �����ű�·��
		std::string script_path = "./source";
		m_script_path = script_path;

		// ��ȡsys.path�б�
		PyObject* sys_path = nullptr;
		PythonRuntime::getGlobal("sys", "path", &sys_path);
		if (Params::logger)
			PyRun_SimpleStringFlags("print sys.path", nullptr);

		if (sys_path && PyList_Check(sys_path)) {
			// �������·��
			PyList_SetSlice(sys_path, 0, PyList_Size(sys_path), nullptr);

			// ���Ӹ���ģ��·��
			PythonRuntime::runMethod(sys_path, "append", src, 0, "(s)", (void*)m_script_path.c_str());

			std::string minecraft_path = m_script_path + "/minecraft";
			PythonRuntime::runMethod(sys_path, "append", src, 0, "(s)", (void*)minecraft_path.c_str());

			std::string framework_path = m_script_path + "/framework";
			PythonRuntime::runMethod(sys_path, "append", src, 0, "(s)", (void*)framework_path.c_str());

			std::string lib_path = m_script_path + "/lib";
			PythonRuntime::runMethod(sys_path, "append", src, 0, "(s)", (void*)lib_path.c_str());

			std::string lobby_path = m_script_path + "/lobby";
			PythonRuntime::runMethod(sys_path, "append", src, 0, "(s)", (void*)lobby_path.c_str());

			std::string mod_path = m_script_path + "/mod";
			PythonRuntime::runMethod(sys_path, "append", src, 0, "(s)", (void*)mod_path.c_str());

			std::string sunshine_path = m_script_path + "/sunshine";
			PythonRuntime::runMethod(sys_path, "append", src, 0, "(s)", (void*)sunshine_path.c_str());

			std::string scripts_path = "./scripts";
			PythonRuntime::runMethod(sys_path, "append", src, 0, "(s)", (void*)scripts_path.c_str());
		}
		if (Params::logger)
			PyRun_SimpleStringFlags("print sys.path", nullptr);
		//PyRun_SimpleString((StringSplitUtils::escape_backslashes("sys.path.append('./source')")).data());
		PyRun_SimpleString("import init");
		//PyImport_ImportModule("init");
	}

	PyGILState_Release(state);
	//PyThreadState* main_state = PyEval_SaveThread();
}

void PythonRuntime::initModules()
{
    LOG(LOG_SCRIPTING, "[PythonRuntime] initModules - Initializing client_instance module");
    initclient_instance();
    LOG(LOG_SCRIPTING, "[PythonRuntime] initModules - Initializing easy_utils module");
    initeasy_utils();
    LOG(LOG_SCRIPTING, "[PythonRuntime] initModules - Initializing client module");
    initclient();
    LOG(LOG_SCRIPTING, "[PythonRuntime] initModules - Initializing tan_game module");
    register_tan_lobby_game_module();
	LOG(LOG_SCRIPTING, "[PythonRuntime] initModules - Initializing websocket module");
	initwebsocket();
	LOG(LOG_SCRIPTING, "[PythonRuntime] initModules - Initializing aes module");
	initaes();
	LOG(LOG_SCRIPTING, "[PythonRuntime] initModules - Initializing chacha module");
	init_chacha();
	LOG(LOG_SCRIPTING, "[PythonRuntime] initModules - Initializing raknet module");
	init_raknet();
	LOG(LOG_SCRIPTING, "[PythonRuntime] initModules - Initializing utility module");
	initutility();
	LOG(LOG_SCRIPTING, "[PythonRuntime] initModules - Initializing setting module");
	initsetting();
	LOG(LOG_SCRIPTING, "[PythonRuntime] initModules - Initializing mod_log module");
	initmod_log();
#if !defined(_DEBUG) && defined(_WIN32)
	LOG(LOG_SCRIPTING, "[PythonRuntime] initModules - Initializing socket module");
	init_socket();
    LOG(LOG_SCRIPTING, "[PythonRuntime] initModules - Initializing select module");
    initselect();
#endif
#ifdef _WIN32
    LOG(LOG_SCRIPTING, "[PythonRuntime] initModules - Initializing ctypes module");
    init_ctypes();
#endif
	LOG(LOG_SCRIPTING, "[PythonRuntime] initModules - Initializing engine module");
	initengine();
	LOG(LOG_SCRIPTING, "[PythonRuntime] initModules - Initializing rotor module");
	initrotor();
	LOG(LOG_SCRIPTING, "[PythonRuntime] initModules - Initializing fop module");
	initfop();
	LOG(LOG_SCRIPTING, "[PythonRuntime] initModules - All modules initialized");
}
