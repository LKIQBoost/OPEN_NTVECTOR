#!/usr/bin/env python3
"""用 minecraft-data 的 blockStates 验证 SubChunk palette 哈希"""
import json, struct, sys
sys.path.insert(0, 'tools')

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

def varint(n):
    out = bytearray()
    while True:
        b = n & 0x7F; n >>= 7
        if n: out.append(b | 0x80)
        else: out.append(b); break
    return bytes(out)

def zig_v(n):
    return varint((n << 1) if n >= 0 else ((-n) << 1) - 1)

def nbt_block(name, states, version, mode='littleVarint'):
    def lp(s):
        if mode == 'littleVarint': return varint(len(s))
        if mode == 'little': return struct.pack('<H', len(s))
        return struct.pack('>H', len(s))
    def nstr(tag, nm, val):
        vb = val.encode()
        return bytes([tag]) + lp(nm) + nm.encode() + lp(val) + vb
    def ncomp(nm, inner):
        return bytes([0x0a]) + lp(nm) + nm.encode() + inner + b'\x00'
    def nint(nm, val):
        if mode == 'littleVarint': v = zig_v(val)
        elif mode == 'little': v = struct.pack('<i', val)
        else: v = struct.pack('>i', val)
        return bytes([0x03]) + lp(nm) + nm.encode() + v
    # states compound
    states_inner = b''
    for k, v in (states or {}).items():
        if isinstance(v, str):
            states_inner += nstr(0x08, k, v)
        elif isinstance(v, bool):
            states_inner += bytes([0x01]) + lp(k) + k.encode() + bytes([1 if v else 0])
        elif isinstance(v, int):
            states_inner += nint(k, v)
    inner = nstr(0x08, 'name', name) + ncomp('states', states_inner) + nint('version', version)
    return bytes([0x0a]) + lp('') + inner + b'\x00'

# 捕获的 palette 值(zigzag 解码)
targets = [0x3b4268e2, 0xdbf44120, 0xc8dc204a, 0x3e6f3450, 0x81eb6d3f, 0x11d6055e, 0xf4694070, 0x157c1374]

bs = json.load(open('build/package/minecraft-data/data/bedrock/1.21.111/blockStates.json'))
print('blockStates:', len(bs))

found = {}
for mode in ['littleVarint', 'little']:
    for prefix in [True, False]:
        for entry in bs:
            name = entry['name']
            nm = ('minecraft:' + name) if prefix else name
            states = entry.get('states') or {}
            version = entry.get('version', 0)
            tag = nbt_block(nm, states, version, mode)
            h = neox(tag)
            if h in targets:
                found.setdefault(h, []).append((name, mode, prefix))
                print('HIT 0x%08x mode=%s prefix=%s block=%s states=%s ver=%d' % (h, mode, prefix, name, states, version))

print('found %d/%d targets' % (len(found), len(targets)))
