#pragma once
#include <cstdint>
#include <string>
#include <algorithm>
#include <vector>
#include <Python.h>

#define CHACHA_ROUNDS 20

// 12字节默认随机数
static const uint8_t DEFAULT_NONCE[12] = {
    '1', '6', '3', ' ', 'N', 'e', 't', 'E', 'a', 's', 'e', '\n'
};

// 16字节默认 IV
static const uint8_t DEFAULT_IV[16] = {
   'e','x','p','a','n','d',' ','3','2','-','b','y','t','e',' ','k'
};

class ChaChaX {
private:
    uint32_t state[16];
    uint8_t block[64];
    uint32_t rounds;
    size_t block_pos;

    // ChaCha 核心 quarterround
    void chacha_quarterround(uint32_t x[16], int a, int b, int c, int d) {
        x[a] += x[b]; x[d] ^= x[a]; x[d] = (x[d] << 16) | (x[d] >> 16);
        x[c] += x[d]; x[b] ^= x[c]; x[b] = (x[b] << 12) | (x[b] >> 20);
        x[a] += x[b]; x[d] ^= x[a]; x[d] = (x[d] << 8) | (x[d] >> 24);
        x[c] += x[d]; x[b] ^= x[c]; x[b] = (x[b] << 7) | (x[b] >> 25);
    }

    // ChaCha 块函数，生成64字节 keystream
    void chacha_block(uint32_t state[16], uint8_t output[64]) {
        uint32_t working_state[16];
        memcpy(working_state, state, sizeof(working_state));

        // 每轮循环执行两轮组合，轮数参数确保1轮也能执行
        for (int i = 0; i < rounds; i += 2) {
            // column rounds
            chacha_quarterround(working_state, 0, 4, 8, 12);
            chacha_quarterround(working_state, 1, 5, 9, 13);
            chacha_quarterround(working_state, 2, 6, 10, 14);
            chacha_quarterround(working_state, 3, 7, 11, 15);
            // diagonal rounds
            chacha_quarterround(working_state, 0, 5, 10, 15);
            chacha_quarterround(working_state, 1, 6, 11, 12);
            chacha_quarterround(working_state, 2, 7, 8, 13);
            chacha_quarterround(working_state, 3, 4, 9, 14);
        }

        for (int i = 0; i < 16; ++i) {
            working_state[i] += state[i];
            memcpy(output + i * 4, &working_state[i], sizeof(uint32_t));
        }
    }

public:
    // 构造函数：传入轮数和 Python bytes 类型的 key
    ChaChaX(uint32_t lv, uint8_t* key_bytes) {
        rounds = lv;
        block_pos = 64; // 初始让第一次 processData 生成新块


        // 安全拷贝 key（32字节）
        memcpy(state + 4, key_bytes, 32);

        state[12] = 0; // 初始计数器

        // 拷贝默认 nonce（12字节）
        memcpy(state + 13, DEFAULT_NONCE, sizeof(DEFAULT_NONCE));

        // 拷贝默认 IV（16字节）
        memcpy(state, DEFAULT_IV, sizeof(DEFAULT_IV));
    }

    // 加密/解密函数，直接作用于字节数组
    void processData(uint8_t* input, size_t length) {
        size_t processed = 0;
        while (processed < length) {
            // 如果需要新的密码块
            if (block_pos >= sizeof(block)) {
                chacha_block(state, block);
                block_pos = 0;
                ++state[12]; // 增加计数器
            }

            // 本次处理字节数
            size_t bytes_available = sizeof(block) - block_pos;
            size_t bytes_to_process = (((bytes_available) < (length - processed)) ? (bytes_available) : (length - processed));

            // 异或操作
            for (size_t i = 0; i < bytes_to_process; ++i) {
                input[processed + i] ^= block[block_pos + i];
            }

            processed += bytes_to_process;
            block_pos += bytes_to_process;
        }
    }
};