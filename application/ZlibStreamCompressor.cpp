#include "ZlibStreamCompressor.h"
#include <stdexcept>
#include <cstring>

ZlibStreamCompressor::ZlibStreamCompressor(int level){
    // 关键步骤：用 memset 清零整个结构体（避免随机值）
    memset(&zs_c, 0, sizeof(z_stream));
    memset(&zs_d, 0, sizeof(z_stream));
    initCompressor(level);
    initDecompressor();
}

ZlibStreamCompressor::~ZlibStreamCompressor() {
    deflateEnd(&zs_c);
    delete[] CompressBuffer;
    inflateEnd(&zs_d);
    delete[] DecompressBuffer;
}

std::vector<uint8_t> ZlibStreamCompressor::Compress(std::vector<uint8_t> data)
{
    std::vector<unsigned char> result;
    Byte* Buffer = CompressBuffer;
    zs_c.next_in = (Byte*)data.data();
    zs_c.avail_in = data.size();
    zs_c.next_out = Buffer;
    zs_c.avail_out = 0x20000;
    int code = 0;
    while (true) {
        deflate(&zs_c, 0);
        if (!zs_c.avail_out) {
            zs_c.next_out = Buffer;
            zs_c.avail_out = 0x20000;
            result.insert(result.end(), Buffer, Buffer + 0x20000);
        }
        if (!zs_c.avail_in)
            break;
    }
    do {
        if (!zs_c.avail_out) {
            zs_c.next_out = Buffer;
            zs_c.avail_out = 0x20000;
            result.insert(result.end(), Buffer, Buffer + 0x20000);
        }
        code = deflate(&zs_c, 2);
    } while (!code);
    result.insert(result.end(), Buffer, Buffer + (0x20000 - zs_c.avail_out));

    return result;
}

std::vector<uint8_t> ZlibStreamCompressor::Decompress(std::vector<uint8_t> data)
{
    std::vector<unsigned char> result;
    //uLong size_ = zs_d.total_out;
    Byte* Buffer = DecompressBuffer;
    zs_d.next_in = (Byte*)data.data();
    zs_d.avail_in = data.size();
    int code = 0;
    while (true) {
        zs_d.next_out = Buffer;
        zs_d.avail_out = 0x10000;
        code = inflate(&zs_d, 0);
        if (code)
            break;
        result.insert(result.end(), Buffer, Buffer + (0x10000 - zs_d.avail_out));
        if (zs_d.avail_out && zs_d.avail_in == code)
            break;
    }
    //result.resize(zs_d.total_out - size_);
    return result;
}

void ZlibStreamCompressor::initCompressor(int level) {
    CompressBuffer = new Byte[0x20000];
    // 使用原始DEFLATE格式（无zlib头尾）
    if (deflateInit2(&zs_c, level, Z_DEFLATED, -MAX_WBITS,
        8, Z_DEFAULT_STRATEGY) != Z_OK) {
        throw std::runtime_error("deflateInit2 failed");
    }
}

void ZlibStreamCompressor::initDecompressor() {
    DecompressBuffer = new Byte[0x10000];
    // 处理原始DEFLATE格式
    if (inflateInit2(&zs_d, -15) != Z_OK) {
        throw std::runtime_error("inflateInit2 failed");
    }
}
