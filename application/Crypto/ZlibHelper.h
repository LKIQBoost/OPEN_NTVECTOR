#pragma once
#include <iostream>
#include <vector>
#include <stdexcept>
//#define ZLIB_WINAPI
#include <zlib.h>

class ZlibHelper {
public:
    // 压缩数据（原始DEFLATE格式，不含zlib头尾）
    static std::vector<unsigned char> compress(const std::vector<unsigned char>& data, int level = Z_DEFAULT_COMPRESSION) {
        z_stream zs;
        memset(&zs, 0, sizeof(zs));

        // 使用负的windowBits来获取原始DEFLATE格式（无zlib头尾）
        if (deflateInit2(&zs, level, Z_DEFLATED, -MAX_WBITS, 6, Z_DEFAULT_STRATEGY) != Z_OK) {
            throw std::runtime_error("deflateInit2 failed");
        }

        zs.next_in = const_cast<Bytef*>(data.data());
        zs.avail_in = data.size();

        int ret;
        std::vector<unsigned char> outbuffer(32768);
        std::vector<unsigned char> output;

        do {
            zs.next_out = outbuffer.data();
            zs.avail_out = outbuffer.size();

            ret = deflate(&zs, Z_FINISH);

            if (output.size() < zs.total_out) {
                output.insert(output.end(),
                    outbuffer.begin(),
                    outbuffer.begin() + (zs.total_out - output.size()));
            }
        } while (ret == Z_OK);

        deflateEnd(&zs);

        if (ret != Z_STREAM_END) {
            throw std::runtime_error("Compression failed");
        }

        return output;
    }

    // 解压数据（原始DEFLATE格式）
    static std::vector<unsigned char> decompress(const std::vector<unsigned char>& compressedData) {
        z_stream zs;
        memset(&zs, 0, sizeof(zs));

        // 使用负的windowBits来处理原始DEFLATE格式
        if (inflateInit2(&zs, -MAX_WBITS) != Z_OK) {
            throw std::runtime_error("inflateInit2 failed");
        }

        zs.next_in = const_cast<Bytef*>(compressedData.data());
        zs.avail_in = compressedData.size();

        int ret;
        std::vector<unsigned char> outbuffer(32768);
        std::vector<unsigned char> output;

        do {
            zs.next_out = outbuffer.data();
            zs.avail_out = outbuffer.size();

            ret = inflate(&zs, 0);

            if (output.size() < zs.total_out) {
                output.insert(output.end(),
                    outbuffer.begin(),
                    outbuffer.begin() + (zs.total_out - output.size()));
            }
        } while (ret == Z_OK);

        inflateEnd(&zs);

        if (ret != Z_STREAM_END) {
            throw std::runtime_error("Decompression failed");
        }

        return output;
    }

    // 字符串压缩的便捷方法
    static std::vector<unsigned char> compressString(const std::string& str, int level = Z_DEFAULT_COMPRESSION) {
        std::vector<unsigned char> data(str.begin(), str.end());
        return compress(data, level);
    }

    // 字符串解压的便捷方法
    static std::string decompressString(const std::vector<unsigned char>& compressedData) {
        auto decompressed = decompress(compressedData);
        return std::string(decompressed.begin(), decompressed.end());
    }
};