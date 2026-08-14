#pragma once

#include <Python.h>

#ifdef __cplusplus
extern "C" {
#endif

	// ������������
	PyObject* decrypt_with_tail(PyObject* self, PyObject* args);
	PyObject* encrypt_with_tail(PyObject* self, PyObject* args);

	// Pythonģ���ʼ������
	//PyMODINIT_FUNC initengine(void);
	PyMODINIT_FUNC PyInit_utility(void);

#ifdef __cplusplus
}
#endif