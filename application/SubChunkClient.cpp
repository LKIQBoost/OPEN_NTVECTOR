#include "SubChunkClient.h"
#include "ClientInstance.h"
#include <chrono>

extern ClientInstance* g_client_instance;

std::mutex SubChunkClient::s_mutex;
std::condition_variable SubChunkClient::s_cv;
bool SubChunkClient::s_pending = false;
std::vector<uint8_t> SubChunkClient::s_response;

// ---- 工具 ----
static void write_varint(std::vector<uint8_t>& out, uint64_t v) {
    while (true) {
        uint8_t b = v & 0x7F;
        v >>= 7;
        if (v) out.push_back(b | 0x80);
        else { out.push_back(b); break; }
    }
}
static void write_zigzag32(std::vector<uint8_t>& out, int32_t v) {
    write_varint(out, (uint32_t)((v << 1) ^ (v >> 31)));
}
static void write_u32le(std::vector<uint8_t>& out, uint32_t v) {
    out.push_back(v & 0xFF); out.push_back((v >> 8) & 0xFF);
    out.push_back((v >> 16) & 0xFF); out.push_back((v >> 24) & 0xFF);
}

bool SubChunkClient::Lookup(uint32_t hash, std::string& name, std::string& states) {
    size_t lo = 0, hi = kBlockRegistryCount;
    while (lo < hi) {
        size_t mid = (lo + hi) / 2;
        if (kBlockRegistry[mid].hash < hash) lo = mid + 1;
        else hi = mid;
    }
    if (lo < kBlockRegistryCount && kBlockRegistry[lo].hash == hash) {
        name = kBlockRegistry[lo].name;
        states = kBlockRegistry[lo].states;
        return true;
    }
    return false;
}

// 解析子区块载荷: v9 + layer_count + y_index + 每层块存储 -> palette 哈希列表
bool SubChunkClient::ParseSubchunk(const std::vector<uint8_t>& payload,
                                   std::vector<uint32_t>& palette) {
    if (payload.empty()) return false;
    Reader r(payload);
    if (!r.avail(3)) return false;
    uint8_t version = r.u8();
    uint8_t layer_count = r.u8();
    if (version >= 9) {
        if (!r.avail(1)) return false;
        r.i8(); // y_index
    }
    for (uint8_t l = 0; l < layer_count; l++) {
        if (!r.avail(1)) return false;
        uint8_t pt = r.u8();
        int bits = pt >> 1;
        if (bits == 0) {
            // 单条目 palette: 读一个 zigzag varint(stateId),无索引
            if (!r.avail(1)) return false;
            uint64_t rid = r.varint();
            palette.push_back((uint32_t)(zigzagv(rid) & 0xFFFFFFFF));
            continue;
        }
        int per_word = 32 / bits;
        size_t words = (4096 + per_word - 1) / per_word;
        if (!r.avail(words * 4)) return false;
        r.skip(words * 4);
        uint64_t psz = zigzagv(r.varint());   // paletteSize (zigzag)
        if (psz > 100000) return false;
        for (uint64_t i = 0; i < psz; i++) {
            uint64_t rid = r.varint();
            palette.push_back((uint32_t)(zigzagv(rid) & 0xFFFFFFFF));
        }
    }
    return true;
}

void SubChunkClient::OnPacket(const std::vector<uint8_t>& payload) {
    std::lock_guard<std::mutex> lk(s_mutex);
    if (!s_pending) return;
    s_response = payload;
    s_pending = false;
    s_cv.notify_all();
}

bool SubChunkClient::RequestBlocks(int ox, int oy, int oz,
                                   const std::vector<std::array<int8_t, 3>>& offsets,
                                   std::vector<BlockData>& out,
                                   int timeout_ms) {
    if (offsets.empty() || !g_client_instance || !g_client_instance->getInstance()) return false;

    // 构建 SubChunkRequest(175)
    std::vector<uint8_t> req;
    write_varint(req, 175);
    write_zigzag32(req, 0);              // dimension 主世界
    write_zigzag32(req, ox);
    write_zigzag32(req, oy);
    write_zigzag32(req, oz);
    write_u32le(req, (uint32_t)offsets.size());
    for (const auto& o : offsets) {
        req.push_back((uint8_t)o[0]);
        req.push_back((uint8_t)o[1]);
        req.push_back((uint8_t)o[2]);
    }

    std::unique_lock<std::mutex> lk(s_mutex);
    bool ok = false;
    // 首个 SubChunkRequest 可能因服务器预热/批处理延迟数秒,超时后重试一次
    for (int attempt = 0; attempt < 3 && !ok; attempt++) {
        s_pending = true;
        s_response.clear();
        g_client_instance->getInstance()->WritePacket(req);  // 发送
        ok = s_cv.wait_for(lk, std::chrono::milliseconds(timeout_ms),
                           [] { return !s_pending; });
        s_pending = false;
    }
    if (!ok) return false;  // 多次超时

    const std::vector<uint8_t>& resp = s_response;
    Reader r(resp);
    if (!r.avail(6)) return false;
    r.u8();                    // cache_enabled
    zigzagv(r.varint());       // dimension
    zigzagv(r.varint());       // origin.x
    zigzagv(r.varint());       // origin.y
    zigzagv(r.varint());       // origin.z
    uint32_t count = r.u32le();
    if (count > offsets.size() * 2 + 16) return false;

    out.clear();
    for (uint32_t i = 0; i < count; i++) {
        if (!r.avail(4)) break;
        int8_t dx = r.i8();
        int8_t dy = r.i8();
        int8_t dz = r.i8();
        uint8_t result = r.u8();
        if (result != 1) {  // 仅 success 有数据
            // 跳过 payload 长度 + 空 payload + heightmaps
            uint64_t plen = r.varint();
            if (!r.avail(plen + 2)) break;
            r.skip(plen);
            uint8_t hm = r.u8();
            if (hm == 1) { if (!r.avail(256)) break; r.skip(256); }
            if (!r.avail(1)) break;
            uint8_t rhm = r.u8();
            if (rhm == 1) { if (!r.avail(256)) break; r.skip(256); }
            continue;
        }
        uint64_t plen = r.varint();
        if (!r.avail(plen)) break;
        const uint8_t* pp = r.take((size_t)plen);
        std::vector<uint8_t> payload(pp, pp + plen);
        // heightmaps
        uint8_t hm = r.u8();
        if (hm == 1) { if (!r.avail(256)) break; r.skip(256); }
        if (!r.avail(1)) break;
        uint8_t rhm = r.u8();
        if (rhm == 1) { if (!r.avail(256)) break; r.skip(256); }

        std::vector<uint32_t> palette;
        if (!ParseSubchunk(payload, palette) || palette.empty()) continue;

        // 块索引: paletteType 的 bits 需要从 payload 再解析一次
        Reader pr(payload);
        if (!pr.avail(3)) continue;
        pr.u8(); pr.u8();
        if (payload[0] >= 9) { if (!pr.avail(1)) continue; pr.i8(); }
        if (!pr.avail(1)) continue;
        uint8_t pt = pr.u8();
        int bits = pt >> 1;
        if (bits == 0) {
            // 全部同一方块
            if (palette.empty()) continue;
            AddBlocks(out, ox + dx, oy + dy, oz + dz, palette[0], palette);
            continue;
        }
        int per_word = 32 / bits;
        size_t words = (4096 + per_word - 1) / per_word;
        if (!pr.avail(words * 4)) continue;
        uint32_t mask = (1u << bits) - 1;
        for (int linear = 0; linear < 4096; linear++) {
            size_t w = linear / per_word;
            int bitoff = (linear % per_word) * bits;
            uint32_t word;
            std::memcpy(&word, pr.p + pr.off + w * 4, 4);
            int pidx = (int)((word >> bitoff) & mask);
            if (pidx < 0 || pidx >= (int)palette.size()) continue;
            uint32_t h = palette[pidx];
            if (h == 0) continue;
            int lx = (linear >> 8) & 0xF;
            int lz = (linear >> 4) & 0xF;
            int ly = linear & 0xF;
            AddBlock(out, (ox + dx) * 16 + lx, (oy + dy) * 16 + ly, (oz + dz) * 16 + lz, h);
        }
    }
    return true;
}

// 辅助: 查找并加入(内部在 cpp 实现,声明在头文件外)
void SubChunkClient::AddBlock(std::vector<BlockData>& out, int x, int y, int z, uint32_t hash) {
    std::string name, states;
    if (!Lookup(hash, name, states)) return;
    if (name == "minecraft:air" || name == "minecraft:cave_air" ||
        name == "minecraft:void_air") return;
    out.push_back({ x, y, z, name, states });
}

void SubChunkClient::AddBlocks(std::vector<BlockData>& out, int sx, int sy, int sz,
                               uint32_t single_hash, const std::vector<uint32_t>& palette) {
    std::string name, states;
    if (!Lookup(single_hash, name, states)) return;
    if (name == "minecraft:air" || name == "minecraft:cave_air" ||
        name == "minecraft:void_air") return;
    for (int lx = 0; lx < 16; lx++)
        for (int ly = 0; ly < 16; ly++)
            for (int lz = 0; lz < 16; lz++)
                out.push_back({ sx * 16 + lx, sy * 16 + ly, sz * 16 + lz, name, states });
}
