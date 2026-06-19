#pragma once
#include <openssl/evp.h>
#include <openssl/err.h>
#include <stdio.h>
#include "Logger.h"

static void calculate_sha256(const char* data, int length, unsigned char* hash) {
    EVP_MD_CTX* mdctx;
    const EVP_MD* md;
    unsigned int hash_len;

    md = EVP_sha256();
    mdctx = EVP_MD_CTX_new();

    if (mdctx == NULL) {
        Logger::getInstance().log(LOG_ERROR, "[Sha256] Failed to create context");
        return;
    }

    if (EVP_DigestInit_ex(mdctx, md, NULL) != 1) {
        Logger::getInstance().log(LOG_ERROR, "[Sha256] Digest initialization failed");
        EVP_MD_CTX_free(mdctx);
        return;
    }

    if (EVP_DigestUpdate(mdctx, data, length) != 1) {
        Logger::getInstance().log(LOG_ERROR, "[Sha256] Digest update failed");
        EVP_MD_CTX_free(mdctx);
        return;
    }

    if (EVP_DigestFinal_ex(mdctx, hash, &hash_len) != 1) {
        Logger::getInstance().log(LOG_ERROR, "[Sha256] Digest finalization failed");
        EVP_MD_CTX_free(mdctx);
        return;
    }

    EVP_MD_CTX_free(mdctx);
}