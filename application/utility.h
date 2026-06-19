#pragma once

#include <Python.h>

#ifdef __cplusplus
extern "C" {
#endif

	// 导出函数声明
	PyObject* decrypt_with_tail(PyObject* self, PyObject* args);
	PyObject* encrypt_with_tail(PyObject* self, PyObject* args);

	// Python模块初始化函数
	//PyMODINIT_FUNC initengine(void);
	void initutility(void);

#ifdef __cplusplus
}
#endif