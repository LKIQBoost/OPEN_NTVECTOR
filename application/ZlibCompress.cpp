#include "ZlibCompress.h"
//#define ZLIB_WINAPI
/*
std::vector<uint8_t> ZlibCompress::Compress(std::vector<uint8_t> data)
{
    void* input = data.data();
    Byte* OutPut = new Byte[data.size()];
    int leng = 0;
    int result = compress(reinterpret_cast<Bytef*>(OutPut), reinterpret_cast<uLongf*>(&leng),
        reinterpret_cast<const Bytef*>(input), static_cast<uLong>(data.size()));
    if (result == Z_OK) {
        return std::vector<uint8_t>(OutPut, OutPut + leng);
    }
    else {
        return std::vector<uint8_t>();
    }
}

std::vector<uint8_t> ZlibCompress::Decompress(std::vector<uint8_t> data)
{
    z_stream stream = {};
    inflateInit(&stream);

    std::vector<uint8_t> output;
    size_t buffer_size = 1024;  // 初始缓冲区大小
    output.resize(buffer_size);

    stream.next_in = data.data();
    stream.avail_in = data.size();
    stream.next_out = output.data();
    stream.avail_out = buffer_size;

    int ret;
    do {
        ret = inflate(&stream, Z_NO_FLUSH);

        if (ret == Z_OK && stream.avail_out == 0) {
            // 缓冲区已满，需要扩容
            size_t old_size = output.size();
            buffer_size *= 2;
            output.resize(buffer_size);

            stream.next_out = output.data() + old_size;
            stream.avail_out = buffer_size - old_size;
        }
    } while (ret == Z_OK);

    if (ret != Z_STREAM_END) {
        return std::vector<uint8_t>();
    }

    // 调整到实际大小
    inflateEnd(&stream);
    return output;
}
*/