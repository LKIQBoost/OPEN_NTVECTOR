#pragma once

#include <Python.h>
#include "NBT/Tag.h"

// Requires the nbt module to have been initialized first.
PyObject* NbtTagToPythonObject(const nbt::Tag& tag);

