#!/usr/bin/env python3
# 验证 SubChunk palette 值是否是方块状态哈希,并确定算法+序列化
import struct, zlib, hashlib

# ---------- 哈希算法 ----------
def _rol32(x, n):
    x &= 0xFFFFFFFF
    return ((x << n) | (x >> (32 - n))) & 0xFFFFFFFF

def _reduce_B(p):
    lo = p & 0xFFFFFFFF; hi = (p >> 32) & 0xFFFFFFFF
    r0 = (lo + (1 if hi else 0)) & 0xFFFFFFFF; c0 = 1 if r0 < lo else 0
    r1 = (r0 + hi) & 0xFFFFFFFF; c1 = 1 if r1 < hi else 0
    return (r1 + c0 + c1) & 0xFFFFFFFF

def _reduce_C(p):
    lo = p & 0xFFFFFFFF; hi = (p >> 32) & 0xFFFFFFFF
    rhi = _rol32(hi, 1); r = (rhi + lo) & 0xFFFFFFFF
    return (r + (2 if r < lo else 0)) & 0xFFFFFFFF

def _round(A, B, C, chunk):
    A = _rol32(A, 1); salt = (A ^ 0x267B0B11) & 0xFFFFFFFF
    xB = (B ^ chunk) & 0xFFFFFFFF; xC = (C ^ chunk) & 0xFFFFFFFF
    m1 = (((xC + salt) & 0xFFFFFFFF & 0xBDEB77DE) | 0x02040801) & 0xFFFFFFFF
    m2 = (((xB + salt) & 0xFFFFFFFF & 0x7D7EBBDE) | 0x00804021) & 0xFFFFFFFF
    B = _reduce_B(m1 * xB); C = _reduce_C(m2 * xC)
    return A, B, C

def _finalize(A, B, C, xin, xout):
    A = _rol32(A, 1); salt = (A ^ 0x267B0B11) & 0xFFFFFFFF
    xB = (B ^ xin) & 0xFFFFFFFF; xC = (C ^ xin) & 0xFFFFFFFF
    m1 = (((xC + salt) & 0xFFFFFFFF & 0xBDEB77DE) | 0x02040801) & 0xFFFFFFFF
    m2 = (((xB + salt) & 0xFFFFFFFF & 0x7D7EBBDE) | 0x00804021) & 0xFFFFFFFF
    B = (_reduce_B(m1 * xB) ^ xout) & 0xFFFFFFFF
    C = (_reduce_C(m2 * xC) ^ xout) & 0xFFFFFFFF
    return A, B, C

def neox(s):
    if isinstance(s, str): s = s.encode()
    L = len(s); A, B, C = 0xF4FA8928, 0x37A8470E, 0x7758B42B; i = 0
    while i + 4 <= L:
        c = struct.unpack_from('<I', s, i)[0]; A, B, C = _round(A, B, C, c); i += 4
    if i < L:
        t = 0
        for j in range(L - i): t |= s[i + j] << (j * 8)
        A, B, C = _round(A, B, C, t)
    A, B, C = _finalize(A, B, C, 0x9BE74448, 0x66F42C48)
    A, B, C = _finalize(A, B, C, 0, 0)
    return (B ^ C) & 0xFFFFFFFF

def fnv1a32(s):
    h = 0x811c9dc5
    for b in s:
        h = ((h ^ b) * 0x01000193) & 0xFFFFFFFF
    return h

def xxh32(s, seed=0):
    PRIME1, PRIME2, PRIME3, PRIME4, PRIME5 = 0x9E3779B1, 0x85EBCA77, 0xC2B2AE3D, 0x27D4EB2F, 0x165667B1
    n = len(s); i = 0
    if n >= 16:
        v1 = (seed + PRIME1 + PRIME2) & 0xFFFFFFFF
        v2 = (seed + PRIME2) & 0xFFFFFFFF
        v3 = seed & 0xFFFFFFFF
        v4 = (seed - PRIME1) & 0xFFFFFFFF
        while i + 16 <= n:
            def rnd(k, v):
                v = (v + struct.unpack_from('<I', s, k)[0] * PRIME2) & 0xFFFFFFFF
                v = (_rol32(v, 13) * PRIME1) & 0xFFFFFFFF
                return v
            v1 = rnd(i, v1); v2 = rnd(i + 4, v2); v3 = rnd(i + 8, v3); v4 = rnd(i + 12, v4)
            i += 16
        h = (_rol32(v1, 1) + _rol32(v2, 7) + _rol32(v3, 12) + _rol32(v4, 18)) & 0xFFFFFFFF
    else:
        h = (seed + PRIME5) & 0xFFFFFFFF
    h = (h + n) & 0xFFFFFFFF
    while i + 4 <= n:
        h = (h + struct.unpack_from('<I', s, i)[0] * PRIME3) & 0xFFFFFFFF
        h = (_rol32(h, 17) * PRIME4) & 0xFFFFFFFF
        i += 4
    while i < n:
        h = (h + s[i] * PRIME5) & 0xFFFFFFFF
        h = (_rol32(h, 11) * PRIME1) & 0xFFFFFFFF
        i += 1
    h ^= h >> 15; h = (h * PRIME2) & 0xFFFFFFFF
    h ^= h >> 13; h = (h * PRIME3) & 0xFFFFFFFF
    h ^= h >> 16
    return h

def hashes(bs):
    out = {}
    out['neox'] = neox(bs)
    out['fnv1a32'] = fnv1a32(bs)
    out['crc32'] = zlib.crc32(bs) & 0xFFFFFFFF
    out['md5lo'] = struct.unpack('<I', hashlib.md5(bs).digest()[:4])[0]
    out['sha1lo'] = struct.unpack('<I', hashlib.sha1(bs).digest()[:4])[0]
    out['xxh32'] = xxh32(bs)
    return out

# ---------- NBT 序列化 ----------
def varint(n):
    out = bytearray()
    while True:
        b = n & 0x7F; n >>= 7
        if n: out.append(b | 0x80)
        else: out.append(b); break
    return bytes(out)

def nbt_block(name, states, version, mode):
    # 构造 {name, states, version} compound
    def lenpre(s):
        if mode == 'little': return struct.pack('<H', len(s))
        if mode == 'littleVarint': return varint(len(s))
        return struct.pack('>H', len(s))
    def nstr(tag, nm, val):
        vb = val.encode()
        return bytes([tag]) + lenpre(nm) + nm.encode() + lenpre(val) + vb
    def ncomp(nm, inner):
        return bytes([0x0a]) + lenpre(nm) + nm.encode() + inner + b'\x00'
    def nint(nm, val):
        if mode == 'little': v = struct.pack('<i', val)
        elif mode == 'littleVarint': v = varint((val << 1) if val >= 0 else ((-val) << 1) - 1)
        else: v = struct.pack('>i', val)
        return bytes([0x03]) + lenpre(nm) + nm.encode() + v
    inner = nstr(0x08, 'name', name) + ncomp('states', b'') + nint('version', version)
    return bytes([0x0a]) + lenpre('') + inner + b'\x00'

def all_serializations(name, version):
    for nm in [name, name.replace('minecraft:', '')]:
        for mode in ['little', 'littleVarint', 'big']:
            for v in [version, 0, 17694720, 18085314]:
                yield ('nbt:%s:ver%d:%s' % (mode, v, nm), nbt_block(nm, {}, v, mode))
        yield ('plain:' + nm, nm.encode())
        yield ('json:' + nm, ('{"name":"%s","states":{}}' % nm).encode())

# ---------- 主逻辑 ----------
targets = {
    'zigzag': [0x3b4268e2, 0xdbf44120, 0xc8dc204a, 0x3e6f3450, 0x81eb6d3f, 0x11d6055e, 0xf4694070, 0x157c1374],
    'plain': [0x768A23C4, 0x482C0D3F, 0xEF06FEAB, 0x7DF9D1A0, 0x23D295BC, 0x1BA4FE9F, 0x2B5AE0E8],
}
blocks = ['minecraft:bedrock','minecraft:deepslate','minecraft:tuff','minecraft:stone','minecraft:granite',
          'minecraft:andesite','minecraft:diorite','minecraft:coal_ore','minecraft:iron_ore','minecraft:copper_ore',
          'minecraft:deepslate_coal_ore','minecraft:deepslate_iron_ore','minecraft:deepslate_copper_ore',
          'minecraft:deepslate_gold_ore','minecraft:deepslate_redstone_ore','minecraft:deepslate_lapis_ore',
          'minecraft:deepslate_diamond_ore','minecraft:deepslate_emerald_ore','minecraft:smooth_basalt',
          'minecraft:calcite','minecraft:amethyst_block','minecraft:gravel','minecraft:dirt','minecraft:raw_iron_block',
          'minecraft:raw_copper_block','minecraft:air','minecraft:lava','minecraft:water','minecraft:magma',
          'minecraft:basalt','minecraft:iron_block','minecraft:redstone_block','minecraft:diamond_block']

for tname, tvals in targets.items():
    for t in tvals:
        for b in blocks:
            for label, bs in all_serializations(b, 0):
                hs = hashes(bs)
                for algo, h in hs.items():
                    if h == t:
                        print('MATCH[%s] target=0x%08x algo=%s block=%s serial=%s' % (tname, t, algo, b, label))
print('done')
