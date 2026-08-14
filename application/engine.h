// engine.h
#pragma once

#include <Python.h>

#ifdef __cplusplus
extern "C" {
#endif

	// ������������
	void trigger_event(const char* event_name, PyObject* args_array);
	void trigger_event_with_args(const char* event_name, int arg_count, ...);

	// Python module init (Python 3)
    PyMODINIT_FUNC PyInit_engine(void);
    PyMODINIT_FUNC PyInit_client_instance(void);

#ifdef __cplusplus
}
#endif
PyMODINIT_FUNC PyInit__client(void);