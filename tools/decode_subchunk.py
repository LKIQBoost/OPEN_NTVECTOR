#!/usr/bin/env python3
# 解码 SubChunk(174) 数据包载荷,验证 block storage 格式
# 用法: python tools/decode_subchunk.py <capture.bin>
import sys
import struct


def read_varint(buf, off):
    res = 0
    shift = 0
    while True:
        b = buf[off]
        off += 1
        res |= (b & 0x7F) << shift
        if not (b & 0x80):
            break
        shift += 7
    return res, off


def zigzag(n):
    return (n >> 1) ^ -(n & 1)


def parse_packet(data):
    off = 0
    cache = data[off]; off += 1
    dim, off = read_varint(data, off); dim = zigzag(dim)
    origin = []
    for _ in range(3):
        v, off = read_varint(data, off)
        origin.append(zigzag(v))
    count = struct.unpack_from('<I', data, off)[0]; off += 4
    entries = []
    for _ in range(count):
        if off + 4 > len(data):
            break  # 数据截断,停止解析剩余 entry
        dx = struct.unpack_from('<b', data, off)[0]; off += 1
        dy = struct.unpack_from('<b', data, off)[0]; off += 1
        dz = struct.unpack_from('<b', data, off)[0]; off += 1
        result = data[off]; off += 1
        plen, off = read_varint(data, off)
        payload = data[off:off + plen]; off += plen
        hm_t = 0
        hm = b''
        rhm_t = 0
        rhm = b''
        if off <= len(data):
            hm_t = data[off]; off += 1
            if hm_t == 1:
                hm = data[off:off + 256]; off += min(256, len(data) - off)
        if off <= len(data):
            rhm_t = data[off]; off += 1
            if rhm_t == 1:
                rhm = data[off:off + 256]; off += min(256, len(data) - off)
        entries.append(dict(dx=dx, dy=dy, dz=dz, result=result,
                            payload=payload, hm_t=hm_t, hm=hm, rhm_t=rhm_t, rhm=rhm))
    return dict(cache=cache, dim=dim, origin=origin, count=count, entries=entries, consumed=off)


def parse_storage(payload, off):
    start = off
    version = payload[off]; off += 1
    bits = version >> 1
    net = bool(version & 1)
    n_words = 0
    if bits > 0:
        per_word = 32 // bits
        n_words = (4096 + per_word - 1) // per_word
        off += n_words * 4
    plen, off = read_varint(payload, off)
    plen = zigzag(plen)  # paletteSize 是 zigzag varint
    palette = []
    for _ in range(plen):
        rid, off = read_varint(payload, off)
        palette.append(zigzag(rid) & 0xFFFFFFFF)  # 条目也是 zigzag varint(32位哈希)
    return dict(version=version, bits=bits, network=net, n_words=n_words,
                palette=palette, bytes=off - start), off


def parse_subchunk(payload):
    """子区块载荷:格式版本9 + 层数 + [v9: y_index] + 各层块存储"""
    if not payload:
        return None
    off = 0
    version = payload[off]; off += 1
    layer_count = payload[off]; off += 1
    y_index = None
    if version == 9:
        y_index = struct.unpack_from('<b', payload, off)[0]; off += 1
    layers = []
    try:
        for _ in range(layer_count):
            storage, off = parse_storage(payload, off)
            layers.append(storage)
    except IndexError:
        pass  # 载荷截断,停止解析剩余层
    return dict(version=version, layer_count=layer_count, y_index=y_index,
                layers=layers, consumed=off, total=len(payload))


RESULT_NAMES = {0: 'undefined', 1: 'success', 2: 'chunk_not_found', 3: 'invalid_dim',
                4: 'player_not_found', 5: 'y_out_of_bounds', 6: 'success_all_air'}


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 1
    data = open(sys.argv[1], 'rb').read()
    p = parse_packet(data)
    print('cache_enabled=%d dimension=%d origin=%s count=%d consumed=%d/%d' % (
        p['cache'], p['dim'], p['origin'], p['count'], p['consumed'], len(data)))
    for i, e in enumerate(p['entries']):
        res = RESULT_NAMES.get(e['result'], e['result'])
        s = parse_subchunk(e['payload'])
        if s:
            layers_desc = []
            for L in s['layers']:
                layers_desc.append('bits=%d net=%s words=%d palette[%d]=%s' % (
                    L['bits'], L['network'], L['n_words'], len(L['palette']), L['palette'][:12]))
            print('  #%d off=(%d,%d,%d) result=%s payload=%dB | v=%d layers=%d y=%s | %s | consumed=%d/%d %s' % (
                i, e['dx'], e['dy'], e['dz'], res, len(e['payload']),
                s['version'], s['layer_count'], s['y_index'],
                '; '.join(layers_desc), s['consumed'], s['total'],
                'OK' if s['consumed'] == s['total'] else '<<< MISMATCH'))
        else:
            print('  #%d off=(%d,%d,%d) result=%s payload=%dB (no data)' % (i, e['dx'], e['dy'], e['dz'], res, len(e['payload'])))
    return 0


if __name__ == '__main__':
    sys.exit(main())
