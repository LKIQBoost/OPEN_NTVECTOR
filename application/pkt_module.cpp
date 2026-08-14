#include "pkt_module.h"
#include <cstring>

// ── varint helpers ──────────────────────────────────────────

static int decode_varint(const unsigned char* buf, Py_ssize_t len,
                         Py_ssize_t off, unsigned int* out) {
    unsigned int result = 0;
    int shift = 0;
    while (off < len) {
        unsigned char b = buf[off++];
        result |= (unsigned int)(b & 0x7F) << shift;
        shift += 7;
        if (!(b & 0x80)) {
            *out = result;
            return (int)off;
        }
        if (shift >= 35) break;
    }
    return -1;
}

static int decode_signed_varint(const unsigned char* buf, Py_ssize_t len,
                                Py_ssize_t off, int* out) {
    unsigned int raw;
    int new_off = decode_varint(buf, len, off, &raw);
    if (new_off < 0) return -1;
    *out = (int)((raw >> 1) ^ -(int)(raw & 1));
    return new_off;
}

static int encode_varint(unsigned int val, unsigned char* out) {
    int n = 0;
    while (val >= 0x80) {
        out[n++] = (unsigned char)(val | 0x80);
        val >>= 7;
    }
    out[n++] = (unsigned char)val;
    return n;
}

// ── bounds check macro ──────────────────────────────────────

#define CHECK_BOUNDS(off, need, len) do { \
    if ((off) < 0 || (off) + (need) > (len)) { \
        PyErr_SetString(PyExc_IndexError, "pkt: read beyond buffer"); \
        return NULL; \
    } \
} while(0)

// ── read functions ──────────────────────────────────────────
// All return (value, new_offset)

static PyObject* pkt_read_byte(PyObject* self, PyObject* args) {
    const char* data; Py_ssize_t dlen; int off;
    if (!PyArg_ParseTuple(args, "y#i", &data, &dlen, &off)) return NULL;
    CHECK_BOUNDS(off, 1, dlen);
    return Py_BuildValue("(ii)", (unsigned char)data[off], off + 1);
}

static PyObject* pkt_read_bool(PyObject* self, PyObject* args) {
    const char* data; Py_ssize_t dlen; int off;
    if (!PyArg_ParseTuple(args, "y#i", &data, &dlen, &off)) return NULL;
    CHECK_BOUNDS(off, 1, dlen);
    PyObject* val = data[off] ? Py_True : Py_False;
    Py_INCREF(val);
    return Py_BuildValue("(Oi)", val, off + 1);
}

static PyObject* pkt_read_varint(PyObject* self, PyObject* args) {
    const char* data; Py_ssize_t dlen; int off;
    if (!PyArg_ParseTuple(args, "y#i", &data, &dlen, &off)) return NULL;
    unsigned int val;
    int new_off = decode_varint((const unsigned char*)data, dlen, off, &val);
    if (new_off < 0) {
        PyErr_SetString(PyExc_ValueError, "pkt: invalid varint");
        return NULL;
    }
    return Py_BuildValue("(Ii)", val, new_off);
}

static PyObject* pkt_read_svarint(PyObject* self, PyObject* args) {
    const char* data; Py_ssize_t dlen; int off;
    if (!PyArg_ParseTuple(args, "y#i", &data, &dlen, &off)) return NULL;
    int val;
    int new_off = decode_signed_varint((const unsigned char*)data, dlen, off, &val);
    if (new_off < 0) {
        PyErr_SetString(PyExc_ValueError, "pkt: invalid signed varint");
        return NULL;
    }
    return Py_BuildValue("(ii)", val, new_off);
}

static PyObject* pkt_read_string(PyObject* self, PyObject* args) {
    const char* data; Py_ssize_t dlen; int off;
    if (!PyArg_ParseTuple(args, "y#i", &data, &dlen, &off)) return NULL;
    unsigned int slen;
    int new_off = decode_varint((const unsigned char*)data, dlen, off, &slen);
    if (new_off < 0) {
        PyErr_SetString(PyExc_ValueError, "pkt: invalid string length varint");
        return NULL;
    }
    CHECK_BOUNDS(new_off, (Py_ssize_t)slen, dlen);
    // MCPE string fields are UTF-8 text: return a proper Py3 str (decoded).
    PyObject* s = PyUnicode_DecodeUTF8(data + new_off, (Py_ssize_t)slen, "surrogateescape");
    return Py_BuildValue("(Oi)", s, new_off + (int)slen);
}

static PyObject* pkt_read_u16(PyObject* self, PyObject* args) {
    const char* data; Py_ssize_t dlen; int off;
    if (!PyArg_ParseTuple(args, "y#i", &data, &dlen, &off)) return NULL;
    CHECK_BOUNDS(off, 2, dlen);
    unsigned short val;
    memcpy(&val, data + off, 2);
    return Py_BuildValue("(Hi)", val, off + 2);
}

static PyObject* pkt_read_u32(PyObject* self, PyObject* args) {
    const char* data; Py_ssize_t dlen; int off;
    if (!PyArg_ParseTuple(args, "y#i", &data, &dlen, &off)) return NULL;
    CHECK_BOUNDS(off, 4, dlen);
    unsigned int val;
    memcpy(&val, data + off, 4);
    return Py_BuildValue("(Ii)", val, off + 4);
}

static PyObject* pkt_read_i32(PyObject* self, PyObject* args) {
    const char* data; Py_ssize_t dlen; int off;
    if (!PyArg_ParseTuple(args, "y#i", &data, &dlen, &off)) return NULL;
    CHECK_BOUNDS(off, 4, dlen);
    int val;
    memcpy(&val, data + off, 4);
    return Py_BuildValue("(ii)", val, off + 4);
}

static PyObject* pkt_read_u64(PyObject* self, PyObject* args) {
    const char* data; Py_ssize_t dlen; int off;
    if (!PyArg_ParseTuple(args, "y#i", &data, &dlen, &off)) return NULL;
    CHECK_BOUNDS(off, 8, dlen);
    unsigned long long val;
    memcpy(&val, data + off, 8);
    return Py_BuildValue("(Ki)", val, off + 8);
}

static PyObject* pkt_read_i64(PyObject* self, PyObject* args) {
    const char* data; Py_ssize_t dlen; int off;
    if (!PyArg_ParseTuple(args, "y#i", &data, &dlen, &off)) return NULL;
    CHECK_BOUNDS(off, 8, dlen);
    long long val;
    memcpy(&val, data + off, 8);
    return Py_BuildValue("(Li)", val, off + 8);
}

static PyObject* pkt_read_f32(PyObject* self, PyObject* args) {
    const char* data; Py_ssize_t dlen; int off;
    if (!PyArg_ParseTuple(args, "y#i", &data, &dlen, &off)) return NULL;
    CHECK_BOUNDS(off, 4, dlen);
    float val;
    memcpy(&val, data + off, 4);
    return Py_BuildValue("(fi)", val, off + 4);
}

static PyObject* pkt_read_f64(PyObject* self, PyObject* args) {
    const char* data; Py_ssize_t dlen; int off;
    if (!PyArg_ParseTuple(args, "y#i", &data, &dlen, &off)) return NULL;
    CHECK_BOUNDS(off, 8, dlen);
    double val;
    memcpy(&val, data + off, 8);
    return Py_BuildValue("(di)", val, off + 8);
}

static PyObject* pkt_read_bytes(PyObject* self, PyObject* args) {
    const char* data; Py_ssize_t dlen; int off; int count;
    if (!PyArg_ParseTuple(args, "y#ii", &data, &dlen, &off, &count)) return NULL;
    CHECK_BOUNDS(off, count, dlen);
    PyObject* s = PyBytes_FromStringAndSize(data + off, count);
    return Py_BuildValue("(Oi)", s, off + count);
}

// ── batch unpack ────────────────────────────────────────────
// pkt.unpack("b?vss", data, offset) -> (val1, val2, ..., new_offset)
//   b = byte(u8)   ? = bool   v = varint   V = signed varint
//   s = string     h = u16    H = i16      i = u32
//   I = i32        q = u64    Q = i64      f = f32    d = f64

static PyObject* pkt_unpack(PyObject* self, PyObject* args) {
    const char* fmt;
    const char* data; Py_ssize_t dlen;
    int off;
    if (!PyArg_ParseTuple(args, "sy#i", &fmt, &data, &dlen, &off)) return NULL;

    const unsigned char* buf = (const unsigned char*)data;
    int fmtlen = (int)strlen(fmt);

    PyObject* result = PyList_New(0);

    for (int fi = 0; fi < fmtlen; fi++) {
        PyObject* val = NULL;
        switch (fmt[fi]) {
        case 'b': {
            CHECK_BOUNDS(off, 1, dlen);
            val = PyLong_FromLong(buf[off++]);
            break;
        }
        case '?': {
            CHECK_BOUNDS(off, 1, dlen);
            val = buf[off++] ? Py_True : Py_False;
            Py_INCREF(val);
            break;
        }
        case 'v': {
            unsigned int v;
            int n = decode_varint(buf, dlen, off, &v);
            if (n < 0) { Py_DECREF(result); PyErr_SetString(PyExc_ValueError, "pkt: invalid varint"); return NULL; }
            off = n;
            val = PyLong_FromUnsignedLong(v);
            break;
        }
        case 'V': {
            int v;
            int n = decode_signed_varint(buf, dlen, off, &v);
            if (n < 0) { Py_DECREF(result); PyErr_SetString(PyExc_ValueError, "pkt: invalid svarint"); return NULL; }
            off = n;
            val = PyLong_FromLong(v);
            break;
        }
        case 's': {
            unsigned int slen;
            int n = decode_varint(buf, dlen, off, &slen);
            if (n < 0 || n + (int)slen > dlen) {
                Py_DECREF(result);
                PyErr_SetString(PyExc_ValueError, "pkt: invalid string");
                return NULL;
            }
            val = PyUnicode_DecodeUTF8(data + n, (Py_ssize_t)slen, "surrogateescape");
            off = n + (int)slen;
            break;
        }
        case 'h': {
            CHECK_BOUNDS(off, 2, dlen);
            unsigned short v; memcpy(&v, data + off, 2);
            val = PyLong_FromLong(v); off += 2;
            break;
        }
        case 'H': {
            CHECK_BOUNDS(off, 2, dlen);
            short v; memcpy(&v, data + off, 2);
            val = PyLong_FromLong(v); off += 2;
            break;
        }
        case 'i': {
            CHECK_BOUNDS(off, 4, dlen);
            unsigned int v; memcpy(&v, data + off, 4);
            val = PyLong_FromUnsignedLong(v); off += 4;
            break;
        }
        case 'I': {
            CHECK_BOUNDS(off, 4, dlen);
            int v; memcpy(&v, data + off, 4);
            val = PyLong_FromLong(v); off += 4;
            break;
        }
        case 'q': {
            CHECK_BOUNDS(off, 8, dlen);
            unsigned long long v; memcpy(&v, data + off, 8);
            val = PyLong_FromUnsignedLongLong(v); off += 8;
            break;
        }
        case 'Q': {
            CHECK_BOUNDS(off, 8, dlen);
            long long v; memcpy(&v, data + off, 8);
            val = PyLong_FromLongLong(v); off += 8;
            break;
        }
        case 'f': {
            CHECK_BOUNDS(off, 4, dlen);
            float v; memcpy(&v, data + off, 4);
            val = PyFloat_FromDouble(v); off += 4;
            break;
        }
        case 'd': {
            CHECK_BOUNDS(off, 8, dlen);
            double v; memcpy(&v, data + off, 8);
            val = PyFloat_FromDouble(v); off += 8;
            break;
        }
        default: {
            Py_DECREF(result);
            PyErr_Format(PyExc_ValueError, "pkt.unpack: unknown format '%c'", fmt[fi]);
            return NULL;
        }
        }
        PyList_Append(result, val);
        Py_DECREF(val);
    }

    PyList_Append(result, PyLong_FromLong(off));
    PyObject* tuple = PyList_AsTuple(result);
    Py_DECREF(result);
    return tuple;
}

// ── write functions ─────────────────────────────────────────

static PyObject* pkt_write_byte(PyObject* self, PyObject* args) {
    int val;
    if (!PyArg_ParseTuple(args, "i", &val)) return NULL;
    unsigned char b = (unsigned char)val;
    return PyBytes_FromStringAndSize((const char*)&b, 1);
}

static PyObject* pkt_write_bool(PyObject* self, PyObject* args) {
    PyObject* val;
    if (!PyArg_ParseTuple(args, "O", &val)) return NULL;
    unsigned char b = PyObject_IsTrue(val) ? 1 : 0;
    return PyBytes_FromStringAndSize((const char*)&b, 1);
}

static PyObject* pkt_write_varint(PyObject* self, PyObject* args) {
    unsigned int val;
    if (!PyArg_ParseTuple(args, "I", &val)) return NULL;
    unsigned char buf[5];
    int n = encode_varint(val, buf);
    return PyBytes_FromStringAndSize((const char*)buf, n);
}

static PyObject* pkt_write_string(PyObject* self, PyObject* args) {
    const char* s; Py_ssize_t slen;
    if (!PyArg_ParseTuple(args, "s#", &s, &slen)) return NULL;
    unsigned char lenbuf[5];
    int n = encode_varint((unsigned int)slen, lenbuf);
    PyObject* result = PyBytes_FromStringAndSize((const char*)lenbuf, n);
    PyBytes_ConcatAndDel(&result, PyBytes_FromStringAndSize(s, slen));
    return result;
}

static PyObject* pkt_write_u16(PyObject* self, PyObject* args) {
    int val;
    if (!PyArg_ParseTuple(args, "i", &val)) return NULL;
    unsigned short v = (unsigned short)val;
    return PyBytes_FromStringAndSize((const char*)&v, 2);
}

static PyObject* pkt_write_u32(PyObject* self, PyObject* args) {
    unsigned int val;
    if (!PyArg_ParseTuple(args, "I", &val)) return NULL;
    return PyBytes_FromStringAndSize((const char*)&val, 4);
}

static PyObject* pkt_write_i32(PyObject* self, PyObject* args) {
    int val;
    if (!PyArg_ParseTuple(args, "i", &val)) return NULL;
    return PyBytes_FromStringAndSize((const char*)&val, 4);
}

static PyObject* pkt_write_u64(PyObject* self, PyObject* args) {
    unsigned long long val;
    if (!PyArg_ParseTuple(args, "K", &val)) return NULL;
    return PyBytes_FromStringAndSize((const char*)&val, 8);
}

static PyObject* pkt_write_f32(PyObject* self, PyObject* args) {
    float val;
    if (!PyArg_ParseTuple(args, "f", &val)) return NULL;
    return PyBytes_FromStringAndSize((const char*)&val, 4);
}

static PyObject* pkt_write_f64(PyObject* self, PyObject* args) {
    double val;
    if (!PyArg_ParseTuple(args, "d", &val)) return NULL;
    return PyBytes_FromStringAndSize((const char*)&val, 8);
}

// ── batch pack ──────────────────────────────────────────────
// pkt.pack("b?vs", 1, True, 100, "hello") -> bytes

static PyObject* pkt_pack(PyObject* self, PyObject* args) {
    Py_ssize_t nargs = PyTuple_Size(args);
    if (nargs < 1) {
        PyErr_SetString(PyExc_TypeError, "pkt.pack requires format string");
        return NULL;
    }
    PyObject* fmt_obj = PyTuple_GetItem(args, 0);
    const char* fmt = PyUnicode_AsUTF8(fmt_obj);
    if (!fmt) return NULL;

    int fmtlen = (int)strlen(fmt);
    if (nargs - 1 < fmtlen) {
        PyErr_SetString(PyExc_TypeError, "pkt.pack: not enough arguments for format");
        return NULL;
    }

    PyObject* parts = PyList_New(0);

    for (int fi = 0; fi < fmtlen; fi++) {
        PyObject* arg = PyTuple_GetItem(args, fi + 1);
        PyObject* chunk = NULL;

        switch (fmt[fi]) {
        case 'b': {
            unsigned char b = (unsigned char)PyLong_AsLong(arg);
            chunk = PyBytes_FromStringAndSize((const char*)&b, 1);
            break;
        }
        case '?': {
            unsigned char b = PyObject_IsTrue(arg) ? 1 : 0;
            chunk = PyBytes_FromStringAndSize((const char*)&b, 1);
            break;
        }
        case 'v': {
            unsigned int v = (unsigned int)PyLong_AsUnsignedLong(arg);
            if (PyErr_Occurred()) { v = (unsigned int)PyLong_AsLong(arg); PyErr_Clear(); }
            unsigned char buf[5];
            int n = encode_varint(v, buf);
            chunk = PyBytes_FromStringAndSize((const char*)buf, n);
            break;
        }
        case 's': {
            // accept a Py3 str (UTF-8) and encode length + utf8 bytes
            Py_ssize_t slen;
            const char* s = PyUnicode_AsUTF8AndSize(arg, &slen);
            if (!s) {
                Py_DECREF(parts); return NULL;
            }
            unsigned char lenbuf[5];
            int n = encode_varint((unsigned int)slen, lenbuf);
            chunk = PyBytes_FromStringAndSize((const char*)lenbuf, n);
            PyBytes_ConcatAndDel(&chunk, PyBytes_FromStringAndSize(s, slen));
            break;
        }
        case 'h': {
            unsigned short v = (unsigned short)PyLong_AsLong(arg);
            chunk = PyBytes_FromStringAndSize((const char*)&v, 2);
            break;
        }
        case 'i': {
            unsigned int v = (unsigned int)PyLong_AsUnsignedLong(arg);
            if (PyErr_Occurred()) { v = (unsigned int)PyLong_AsLong(arg); PyErr_Clear(); }
            chunk = PyBytes_FromStringAndSize((const char*)&v, 4);
            break;
        }
        case 'I': {
            int v = (int)PyLong_AsLong(arg);
            chunk = PyBytes_FromStringAndSize((const char*)&v, 4);
            break;
        }
        case 'f': {
            float v = (float)PyFloat_AsDouble(arg);
            chunk = PyBytes_FromStringAndSize((const char*)&v, 4);
            break;
        }
        case 'd': {
            double v = PyFloat_AsDouble(arg);
            chunk = PyBytes_FromStringAndSize((const char*)&v, 8);
            break;
        }
        default:
            Py_DECREF(parts);
            PyErr_Format(PyExc_ValueError, "pkt.pack: unknown format '%c'", fmt[fi]);
            return NULL;
        }
        PyList_Append(parts, chunk);
        Py_DECREF(chunk);
    }

    PyObject* empty = PyBytes_FromStringAndSize("", 0);
    PyObject* result = _PyBytes_Join(empty, parts);
    Py_DECREF(empty);
    Py_DECREF(parts);
    return result;
}

// ── remaining / length helper ───────────────────────────────

static PyObject* pkt_remaining(PyObject* self, PyObject* args) {
    const char* data; Py_ssize_t dlen; int off;
    if (!PyArg_ParseTuple(args, "y#i", &data, &dlen, &off)) return NULL;
    return PyLong_FromLong(dlen - off);
}

// ── module def ──────────────────────────────────────────────

static PyMethodDef PktMethods[] = {
    {"read_byte",    pkt_read_byte,    METH_VARARGS, "read_byte(data, off) -> (val, off)"},
    {"read_bool",    pkt_read_bool,    METH_VARARGS, "read_bool(data, off) -> (val, off)"},
    {"read_varint",  pkt_read_varint,  METH_VARARGS, "read_varint(data, off) -> (val, off)"},
    {"read_svarint", pkt_read_svarint, METH_VARARGS, "read_svarint(data, off) -> (val, off)"},
    {"read_string",  pkt_read_string,  METH_VARARGS, "read_string(data, off) -> (str, off)"},
    {"read_u16",     pkt_read_u16,     METH_VARARGS, "read_u16(data, off) -> (val, off)"},
    {"read_u32",     pkt_read_u32,     METH_VARARGS, "read_u32(data, off) -> (val, off)"},
    {"read_i32",     pkt_read_i32,     METH_VARARGS, "read_i32(data, off) -> (val, off)"},
    {"read_u64",     pkt_read_u64,     METH_VARARGS, "read_u64(data, off) -> (val, off)"},
    {"read_i64",     pkt_read_i64,     METH_VARARGS, "read_i64(data, off) -> (val, off)"},
    {"read_f32",     pkt_read_f32,     METH_VARARGS, "read_f32(data, off) -> (val, off)"},
    {"read_f64",     pkt_read_f64,     METH_VARARGS, "read_f64(data, off) -> (val, off)"},
    {"read_bytes",   pkt_read_bytes,   METH_VARARGS, "read_bytes(data, off, count) -> (bytes, off)"},
    {"unpack",       pkt_unpack,       METH_VARARGS, "unpack(fmt, data, off) -> (v1, v2, ..., off)"},
    {"write_byte",   pkt_write_byte,   METH_VARARGS, "write_byte(val) -> bytes"},
    {"write_bool",   pkt_write_bool,   METH_VARARGS, "write_bool(val) -> bytes"},
    {"write_varint", pkt_write_varint, METH_VARARGS, "write_varint(val) -> bytes"},
    {"write_string", pkt_write_string, METH_VARARGS, "write_string(s) -> bytes"},
    {"write_u16",    pkt_write_u16,    METH_VARARGS, "write_u16(val) -> bytes"},
    {"write_u32",    pkt_write_u32,    METH_VARARGS, "write_u32(val) -> bytes"},
    {"write_i32",    pkt_write_i32,    METH_VARARGS, "write_i32(val) -> bytes"},
    {"write_u64",    pkt_write_u64,    METH_VARARGS, "write_u64(val) -> bytes"},
    {"write_f32",    pkt_write_f32,    METH_VARARGS, "write_f32(val) -> bytes"},
    {"write_f64",    pkt_write_f64,    METH_VARARGS, "write_f64(val) -> bytes"},
    {"pack",         pkt_pack,         METH_VARARGS, "pack(fmt, ...) -> bytes"},
    {"remaining",    pkt_remaining,    METH_VARARGS, "remaining(data, off) -> int"},
    {NULL, NULL, 0, NULL}
};

// Module init (Python 3)
static struct PyModuleDef pkt_module = { PyModuleDef_HEAD_INIT, "pkt", NULL, -1, PktMethods };
PyMODINIT_FUNC PyInit_pkt(void) {
    return PyModule_Create(&pkt_module);
}
