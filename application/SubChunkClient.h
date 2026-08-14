#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <array>
#include <mutex>
#include <condition_variable>
#include <algorithm>
#include <cstring>
#include "BlockRegistry.h"

// MCBE 1.21.120 SubChunk 数据抓取(基于 FNV-1a 块哈希,见 tools/gen_block_registry.py)
//
// 块哈希 = FNV-1a32 over little-NBT {name, states}
// SubChunk(174) 载荷: v9 + layer_count + y_index + 每层块存储
//   块存储: paletteType(bits=ver>>1, net=ver&1) + 索引(words*4) + paletteSize(zigzag) + palette(zigzag 哈希)

class SubChunkClient {
public:
    struct BlockData {
        int x, y, z;
        std::string name;       // e.g. "minecraft:deepslate"
        std::string states_json; // e.g. {"pillar_axis":"y"}
    };

    // 发送 SubChunkRequest 并等待响应,解析出所有非空气方块(绝对世界坐标)
    static bool RequestBlocks(int ox, int oy, int oz,
                              const std::vector<std::array<int8_t, 3>>& offsets,
                              std::vector<BlockData>& out,
                              int timeout_ms = 6000);

    // 由数据包回调调用
    static void OnPacket(const std::vector<uint8_t>& payload);

private:
    // ---- 小端读取辅助 ----
    struct Reader {
        const uint8_t* p;
        size_t n;
        size_t off = 0;
        Reader(const std::vector<uint8_t>& d) : p(d.data()), n(d.size()) {}
        bool avail(size_t k) const { return off + k <= n; }
        uint8_t u8() { return p[off++]; }
        int8_t i8() { return (int8_t)p[off++]; }
        uint32_t u32le() { uint32_t v; std::memcpy(&v, p + off, 4); off += 4; return v; }
        uint64_t varint() {
            uint64_t v = 0; int sh = 0;
            while (true) {
                uint8_t b = u8();
                v |= (uint64_t)(b & 0x7F) << sh;
                if (!(b & 0x80)) break;
                sh += 7;
            }
            return v;
        }
        void skip(size_t k) { off += k; }
        const uint8_t* take(size_t k) { const uint8_t* r = p + off; off += k; return r; }
    };

    static int64_t zigzagv(uint64_t v) { return (int64_t)(v >> 1) ^ -(int64_t)(v & 1); }

    // 反查注册表(二分)
    static bool Lookup(uint32_t hash, std::string& name, std::string& states);

    // 解析子区块载荷 -> palette 哈希列表
    static bool ParseSubchunk(const std::vector<uint8_t>& payload,
                              std::vector<uint32_t>& palette);

    static void AddBlock(std::vector<BlockData>& out, int x, int y, int z, uint32_t hash);
    static void AddBlocks(std::vector<BlockData>& out, int sx, int sy, int sz,
                          uint32_t single_hash, const std::vector<uint32_t>& palette);

    // 全局请求状态
    static std::mutex s_mutex;
    static std::condition_variable s_cv;
    static bool s_pending;
    static std::vector<uint8_t> s_response;
};
