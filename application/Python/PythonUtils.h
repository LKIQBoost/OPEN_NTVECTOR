#pragma once

#include <Python.h>
class PythonUtils
{
public:
	static PyCodeObject* ReadMarshalCodeObject(char* code,int length);
};