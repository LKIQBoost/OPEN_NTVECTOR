#pragma once

#include <Python.h>

extern "C" {
PyMODINIT_FUNC PyInit_nbt(void);
PyMODINIT_FUNC PyInit_packets(void);
PyMODINIT_FUNC PyInit_game_state(void);
PyMODINIT_FUNC PyInit_api_errors(void);
}
