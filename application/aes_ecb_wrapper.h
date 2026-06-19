#ifndef AES_ECB_H
#define AES_ECB_H

#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

// 初始化OpenSSL（线程安全，可多次调用）
void initaes(void);

#ifdef __cplusplus
}
#endif

#endif