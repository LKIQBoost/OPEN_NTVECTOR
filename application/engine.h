// engine.h
#pragma once

#include <Python.h>

#ifdef __cplusplus
extern "C" {
#endif

	// 导出函数声明
	void trigger_event(const char* event_name, PyObject* args_array);
	void trigger_event_with_args(const char* event_name, int arg_count, ...);

	// Python模块初始化函数
    void initengine(void);
    void initclient_instance(void);

#ifdef __cplusplus
}
#endif
void initclient(void);