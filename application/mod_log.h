#pragma once

#include <Python.h>

#ifdef __cplusplus
extern "C" {
#endif

	// Python模块初始化函数
	//PyMODINIT_FUNC initengine(void);
	void initmod_log(void);

#ifdef __cplusplus
}
#endif