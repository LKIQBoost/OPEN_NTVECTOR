#include "aes_ecb_wrapper.h"
// aes_module.c
#include <Python.h>
#include <openssl/evp.h>
#include <openssl/err.h>

// AES-128-ECB 加密函数
static PyObject* aes_ecb128encrypt(PyObject* self, PyObject* args) {
    const char* key;
    const char* data;
    int key_len, data_len;

    if (!PyArg_ParseTuple(args, "s#s#", &key, &key_len, &data, &data_len)) {
        return NULL;
    }

    // 检查长度
    if (key_len != 16) {
        PyErr_SetString(PyExc_ValueError, "Key must be 16 bytes");
        return NULL;
    }
    if (data_len != 16) {
        PyErr_SetString(PyExc_ValueError, "Data must be 16 bytes");
        return NULL;
    }

    // 准备输出
    unsigned char output[16];

    // OpenSSL 加密
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to create cipher context");
        return NULL;
    }

    if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_128_ecb(), NULL, (const unsigned char*)key, NULL)) {
        EVP_CIPHER_CTX_free(ctx);
        PyErr_SetString(PyExc_RuntimeError, "Failed to initialize encryption");
        return NULL;
    }

    EVP_CIPHER_CTX_set_padding(ctx, 0);

    int out_len = 0;
    if (1 != EVP_EncryptUpdate(ctx, output, &out_len, (const unsigned char*)data, 16)) {
        EVP_CIPHER_CTX_free(ctx);
        PyErr_SetString(PyExc_RuntimeError, "Encryption failed");
        return NULL;
    }

    EVP_CIPHER_CTX_free(ctx);

    // 返回 bytes
    return PyBytes_FromStringAndSize((char*)output, 16);
}

// AES-128-ECB 解密函数
static PyObject* aes_ecb128decrypt(PyObject* self, PyObject* args) {
    const char* key;
    const char* data;
    int key_len, data_len;

    if (!PyArg_ParseTuple(args, "s#s#", &key, &key_len, &data, &data_len)) {
        return NULL;
    }

    if (key_len != 16) {
        PyErr_SetString(PyExc_ValueError, "Key must be 16 bytes");
        return NULL;
    }
    if (data_len != 16) {
        PyErr_SetString(PyExc_ValueError, "Data must be 16 bytes");
        return NULL;
    }

    unsigned char output[16];

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to create cipher context");
        return NULL;
    }

    if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_128_ecb(), NULL, (const unsigned char*)key, NULL)) {
        EVP_CIPHER_CTX_free(ctx);
        PyErr_SetString(PyExc_RuntimeError, "Failed to initialize decryption");
        return NULL;
    }

    EVP_CIPHER_CTX_set_padding(ctx, 0);

    int out_len = 0;
    if (1 != EVP_DecryptUpdate(ctx, output, &out_len, (const unsigned char*)data, 16)) {
        EVP_CIPHER_CTX_free(ctx);
        PyErr_SetString(PyExc_RuntimeError, "Decryption failed");
        return NULL;
    }

    EVP_CIPHER_CTX_free(ctx);

    return PyBytes_FromStringAndSize((char*)output, 16);
}

// 模块方法表
static PyMethodDef AesMethods[] = {
    {"ecb128encrypt", aes_ecb128encrypt, METH_VARARGS, "AES-128-ECB encrypt (16 bytes key, 16 bytes data)"},
    {"ecb128decrypt", aes_ecb128decrypt, METH_VARARGS, "AES-128-ECB decrypt (16 bytes key, 16 bytes data)"},
    {NULL, NULL, 0, NULL}
};

// 模块初始化函数 (Python 2.7)
PyMODINIT_FUNC initaes(void) {
    Py_InitModule("aes", AesMethods);

    // 初始化 OpenSSL
    OpenSSL_add_all_algorithms();
    ERR_load_crypto_strings();
}