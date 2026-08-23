#pragma once
//#define ZLIB_WINAPI
/*
#include <zlib.h>
#include <vector>
class ZlibCompress
{
public:
	virtual ~ZlibCompress() = default;
	virtual std::vector<uint8_t> Compress(std::vector<uint8_t> data) ;
	virtual std::vector<uint8_t> Decompress(std::vector<uint8_t> data);
};
*/
#pragma once
#include <vector>
#include <cstdint>
#include <stdexcept>
#include <zlib.h>
#include "Logger.h"

class ZlibCompress
{
public:
    virtual ~ZlibCompress() = default;

    virtual std::vector<uint8_t> Compress(std::vector<uint8_t> data) {
        if (data.empty()) {
            return std::vector<uint8_t>();
        }

        z_stream stream;
        stream.zalloc = Z_NULL;
        stream.zfree = Z_NULL;
        stream.opaque = Z_NULL;

        int ret = deflateInit2(&stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED,
            -15, 8, Z_DEFAULT_STRATEGY);
        if (ret != Z_OK) {
            Logger::getInstance().log(LOG_NETWORK, "deflateInit2 failed");
            return std::vector<uint8_t>();
        }

        // 设置输入数据
        stream.avail_in = static_cast<uInt>(data.size());
        stream.next_in = reinterpret_cast<Bytef*>(data.data());

        // 预分配结果缓冲区：压缩数据最大大小 + 1字节头部
        std::vector<uint8_t> result;
        size_t maxCompressedSize = deflateBound(&stream, static_cast<uLong>(data.size()));
        result.resize(maxCompressedSize + 1);  // +1 为头部00字节预留空间

        // 在第一个位置写入00字节
        result[0] = 0x00;

        // 设置输出：从第二个字节开始
        stream.avail_out = static_cast<uInt>(maxCompressedSize);
        stream.next_out = reinterpret_cast<Bytef*>(result.data() + 1);

        // 执行压缩
        ret = deflate(&stream, Z_FINISH);
        deflateEnd(&stream);

        if (ret != Z_STREAM_END) {
            Logger::getInstance().log(LOG_NETWORK, "deflate failed: " + std::to_string(ret));
            return std::vector<uint8_t>();
        }

        // 调整大小为：1字节头部 + 实际压缩数据大小
        result.resize(stream.total_out + 1);

        return result;
    }

    virtual std::vector<uint8_t> Decompress(std::vector<uint8_t> data) {
        if (data.empty()) {
            return std::vector<uint8_t>();
        }

        // 根据头部字节进行不同处理
        uint8_t header = data[0];

        if (header == 0xFF) {
            // 头部是FF：直接返回去掉FF的数据
            if (data.size() <= 1) {
                return std::vector<uint8_t>(); // 只有FF字节，返回空
            }
            return std::vector<uint8_t>(data.begin() + 1, data.end());
        }
        else if (header == 0x00) {
            // 头部是00：进行DEFLATE解压
            if (data.size() <= 1) {
                return std::vector<uint8_t>(); // 只有00字节，返回空
            }

            z_stream stream;
            stream.zalloc = Z_NULL;
            stream.zfree = Z_NULL;
            stream.opaque = Z_NULL;
            stream.avail_in = 0;
            stream.next_in = Z_NULL;

            // 使用原始 DEFLATE 格式（无 zlib 头尾）
            int ret = inflateInit2(&stream, -15);
            if (ret != Z_OK) {
                Logger::getInstance().log(LOG_NETWORK, "inflateInit2 failed");
            }

            // 设置输入数据（跳过头部的00字节）
            stream.avail_in = static_cast<uInt>(data.size() - 1);
            stream.next_in = reinterpret_cast<Bytef*>(data.data() + 1);

            // 准备输出缓冲区
            std::vector<uint8_t> decompressed;
            decompressed.resize((data.size() - 1) * 4); // 初始估计大小

            stream.avail_out = static_cast<uInt>(decompressed.size());
            stream.next_out = reinterpret_cast<Bytef*>(decompressed.data());

            int flush = Z_NO_FLUSH;

            do {
                if (stream.avail_out == 0) {
                    // 扩大输出缓冲区
                    size_t old_size = decompressed.size();
                    decompressed.resize(old_size * 2);
                    stream.next_out = reinterpret_cast<Bytef*>(decompressed.data() + stream.total_out);
                    stream.avail_out = static_cast<uInt>(decompressed.size() - stream.total_out);
                }

                ret = inflate(&stream, flush);

                if (ret != Z_OK && ret != Z_STREAM_END && ret != Z_BUF_ERROR) {
                    inflateEnd(&stream);
                    Logger::getInstance().log(LOG_NETWORK, "inflate failed: " + std::to_string(ret));
                }

            } while (ret != Z_STREAM_END);

            // 清理资源
            inflateEnd(&stream);

            // 调整大小为实际解压数据
            decompressed.resize(stream.total_out);

            return decompressed;
        }
        else {
            return std::vector<uint8_t>();
        }
    }
    std::vector<uint8_t> MCPDecompress(std::vector<uint8_t> data)
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
};
