#ifndef AES_ECB_H
#define AES_ECB_H

#include <Python.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

// ��ʼ��OpenSSL���̰߳�ȫ���ɶ�ε��ã�
PyMODINIT_FUNC PyInit_aes(void);

#ifdef __cplusplus
}
#endif

#endif