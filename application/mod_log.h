#pragma once

#include <Python.h>

#ifdef __cplusplus
extern "C" {
#endif

	// Pythonģ���ʼ������
	//PyMODINIT_FUNC initengine(void);
	PyMODINIT_FUNC PyInit_mod_log(void);

#ifdef __cplusplus
}
#endif