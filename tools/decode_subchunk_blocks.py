#!/usr/bin/env python3
"""用 FNV-1a 哈希解码 SubChunk palette -> 方块名"""
import json, struct, sys, os
sys.path.insert(0, 'tools')
from decode_subchunk import parse_packet, parse_subchunk


def fnv1a32(s):
    h = 0x811c9dc5
    for b in s:
        h = ((h ^ b) * 0x01000193) & 0xFFFFFFFF
    return h


def varint(n):
    out = bytearray()
    while True:
        b = n & 0x7F; n >>= 7
        if n: out.append(b | 0x80)
        else: out.append(b); break
    return bytes(out)


def nbt_little_string(name, val):
    vb = val.encode()
    return bytes([0x08]) + struct.pack('<H', len(name)) + name.encode() + struct.pack('<H', len(vb)) + vb


def nbt_little_compound(name, inner):
    return bytes([0x0a]) + struct.pack('<H', len(name)) + name.encode() + inner + b'\x00'


def nbt_little_int(name, val):
    return bytes([0x03]) + struct.pack('<H', len(name)) + name.encode() + struct.pack('<i', val)


def nbt_little_byte(name, val):
    return bytes([0x01]) + struct.pack('<H', len(name)) + name.encode() + bytes([val])


def block_hash(name_with_ns, states):
    # states: dict
    inner = b''
    for k, v in (states or {}).items():
        if isinstance(v, str):
            inner += nbt_little_string(k, v)
        elif isinstance(v, bool):
            inner += nbt_little_byte(k, 1 if v else 0)
        elif isinstance(v, int):
            inner += nbt_little_int(k, v)
    states_comp = nbt_little_compound('states', inner)
    body = nbt_little_string('name', name_with_ns) + states_comp
    tag = bytes([0x0a]) + struct.pack('<H', 0) + body + b'\x00'
    return fnv1a32(tag)


def build_map(blockstates_path):
    bs = json.load(open(blockstates_path))
    hmap = {}
    for e in bs:
        nm = 'minecraft:' + e['name']
        raw_states = e.get('states') or {}
        # states 是元数据格式 {key: {type, value}} -> 提取 value
        states = {k: v['value'] if isinstance(v, dict) and 'value' in v else v
                  for k, v in raw_states.items()}
        h = block_hash(nm, states)
        hmap.setdefault(h, []).append((e['name'], states))
    return hmap


def decode_palette(hashes, hmap):
    out = []
    for h in hashes:
        entries = hmap.get(h)
        if entries:
            nm = entries[0][0]
            st = entries[0][1] if len(entries) == 1 else [e[0] for e in entries]
        else:
            nm = '??0x%08x' % h
            st = None
        out.append((h, nm, st))
    return out


if __name__ == '__main__':
    cap = sys.argv[1] if len(sys.argv) > 1 else r'D:/Game/Minecraft/Vector/scripts/subchunk_probe/captures/subchunk_1786684845787.bin'
    bspath = sys.argv[2] if len(sys.argv) > 2 else 'build/package/minecraft-data/data/bedrock/1.21.111/blockStates.json'
    data = open(cap, 'rb').read()
    p = parse_packet(data)
    hmap = build_map(bspath)
    print('block registry:', len(hmap), 'hashes')
    unknown = set()
    for i, e in enumerate(p['entries']):
        if not e['payload']:
            print('#%d off=(%d,%d,%d) %s' % (i, e['dx'], e['dy'], e['dz'], 'all_air'))
            continue
        s = parse_subchunk(e['payload'])
        if not s or not s['layers']:
            print('#%d off=(%d,%d,%d) %s' % (i, e['dx'], e['dy'], e['dz'], 'no-layer?'))
            continue
        layer = s['layers'][0]
        hashes = layer['palette']
        dec = decode_palette(hashes, hmap)
        names = [d[1] for d in dec]
        for h, nm, st in dec:
            if nm.startswith('??'):
                unknown.add(nm)
        print('#%d off=(%d,%d,%d) y=%s bits=%d palette=%s' % (i, e['dx'], e['dy'], e['dz'], s['y_index'], layer['bits'], names))
    if unknown:
        print('UNKNOWN hashes:', unknown)
