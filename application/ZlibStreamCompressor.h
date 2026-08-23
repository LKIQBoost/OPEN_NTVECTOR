#pragma once

#include <vector>
//#define ZLIB_WINAPI
#include <zlib.h>
#include "ZlibCompress.h"

class ZlibStreamCompressor : public ZlibCompress{
public:
    explicit ZlibStreamCompressor(int level = Z_DEFAULT_COMPRESSION);
    ~ZlibStreamCompressor();

    // 处理数据块，返回处理后的数据
    std::vector<uint8_t> Compress(std::vector<uint8_t> data) override;
    std::vector<uint8_t> Decompress(std::vector<uint8_t> data) override;

    // 禁用拷贝和赋值
    ZlibStreamCompressor(const ZlibStreamCompressor&) = delete;
    ZlibStreamCompressor& operator=(const ZlibStreamCompressor&) = delete;

private:
    void initCompressor(int level);
    void initDecompressor();

    z_stream zs_d;
    z_stream zs_c;
    bool finished_ = false;
    Byte* CompressBuffer;
    Byte* DecompressBuffer;
};


