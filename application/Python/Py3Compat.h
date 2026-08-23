#pragma once
// Py3Compat.h - helpers for the Python 2 -> Python 3 migration.
// Centralizes the GBK <-> UTF-8 boundary so Chinese text round-trips
// correctly between C++ (GBK/ANSI) and Python 3 (Unicode).
//
// NOTE: encoding is done with Python's built-in 'gbk' codec (PyUnicode_Decode /
// PyUnicode_AsEncodedString), NOT windows.h. This deliberately avoids pulling
// <windows.h> into the same TU as headers that do `using namespace std;`,
// which would collide Windows SDK's `byte` typedef with C++17 `std::byte`.
#include <Python.h>
#include <string>

// Build a Python 3 str from a GBK/ANSI C++ string (config, CLI, local text).
inline PyObject* PyTextFromGbk(const std::string& gbk) {
    if (gbk.empty()) return PyUnicode_FromString("");
    return PyUnicode_Decode(gbk.data(), (Py_ssize_t)gbk.size(), "gbk", "replace");
}
inline PyObject* PyTextFromGbk(const char* s) {
    return s ? PyTextFromGbk(std::string(s)) : PyUnicode_FromString("");
}
inline PyObject* PyTextFromGbk(const char* s, Py_ssize_t n) {
    return PyUnicode_Decode(s, n, "gbk", "replace");
}

// Build a Python 3 str from UTF-8 data (protocol text, JSON, etc.) with no recode.
inline PyObject* PyTextFromUtf8(const char* s) {
    return PyUnicode_FromString(s ? s : "");
}
// Length variant: never raises on stray bytes (surrogateescape), safe for packet text.
inline PyObject* PyTextFromUtf8(const char* s, Py_ssize_t n) {
    return PyUnicode_DecodeUTF8(s, n, "surrogateescape");
}

// Convert a Python 3 str back to a GBK/ANSI std::string for C++ consumers.
inline std::string PyGbkFromText(PyObject* o) {
    if (!o || !PyUnicode_Check(o)) return std::string();
    PyObject* b = PyUnicode_AsEncodedString(o, "gbk", "replace");
    if (!b) { PyErr_Clear(); return std::string(); }
    std::string r(PyBytes_AsString(b), (size_t)PyBytes_Size(b));
    Py_DECREF(b);
    return r;
}

// Convert a Python 3 str to UTF-8 std::string (already Unicode on the Python side).
inline std::string PyUtf8FromText(PyObject* o) {
    if (!o || !PyUnicode_Check(o)) return std::string();
    const char* u = PyUnicode_AsUTF8(o);
    return u ? std::string(u) : std::string();
}

// Convert a UTF-8 C string to a GBK/ANSI std::string (for GBK consoles/loggers).
inline std::string Utf8ToGbk(const char* s) {
    if (!s) return std::string();
    PyObject* u = PyUnicode_FromString(s);
    if (!u) { PyErr_Clear(); return std::string(s); }
    std::string r = PyGbkFromText(u);
    Py_DECREF(u);
    return r;
}
