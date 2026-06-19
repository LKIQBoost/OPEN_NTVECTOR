#pragma once
#include <vector>
#include <Python.h>
#include <string>
#include "MCPFileSystem.h"
#include "Logger.h"
#include "raknet_wrapper.h"
#include "aes_ecb_wrapper.h"
#include "websocket_wrapper.h"
#include "ZlibCompress.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wregister"
#include "rotormodule.c"
#pragma GCC diagnostic pop
#include "chacha_wrapper.h"
#include "ConfigLoader.h"
#include "PythonUtils.h"
#include "StringSplitUtils.h"

#define VANILLA_MCP "vanilla.mcp"
class PythonRuntime
{
public:
	static std::string m_script_path;
	static std::map<std::string, MCPFileSystem> mod_file_system_map;
	static MCPFileSystem m_file_system;
	static std::vector<uint8_t> m_last_file;
	static std::string m_last_filename;
	static int runMethod(
		PyObject* object,
		const char* method_name,
		const char* result_format,
		void* result,
		const char* args_format,
		...);
	static int runMethodNoGIL(
		PyObject* object,
		const char* method_name,
		const char* result_format,
		void* result,
		const char* args_format,
		...);
	static int getGlobal(
		const char* module_name,
		const char* var_name,
		PyObject** result);
	static int getGlobalNoGIL(
			const char* module_name,
			const char* var_name,
			PyObject** result);
	static std::vector<uint8_t> openFile(const std::string& fullname);
	static int convertResult(PyObject* py_res, const char* result_format, void* result);
	static int convertResultNoGIL(PyObject* py_res, const char* result_format, void* result);
	static void startUp();
	static void initModules();
};

