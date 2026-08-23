#!/usr/bin/env python3
"""JSON 驱动的 MCBE 协议解码器(基于 minecraft-data types)。

用法:
  from proto_decode import Decoder, load_types
  d = Decoder(data, types)
  val, off = d.decode('packet_start_game', 0)
"""
import json
import struct


def load_types(path):
    with open(path, 'r', encoding='utf-8') as f:
        return json.load(f)['types']


class Decoder:
    def __init__(self, data, types):
        self.data = data
        self.types = types

    # ---- 原始读取 ----
    def u8(self, off):
        return self.data[off], off + 1

    def i8(self, off):
        return struct.unpack_from('<b', self.data, off)[0], off + 1

    def u16(self, off):
        return struct.unpack_from('<H', self.data, off)[0], off + 2

    def u32(self, off):
        return struct.unpack_from('<I', self.data, off)[0], off + 4

    def i32(self, off):
        return struct.unpack_from('<i', self.data, off)[0], off + 4

    def u64(self, off):
        return struct.unpack_from('<Q', self.data, off)[0], off + 8

    def i64(self, off):
        return struct.unpack_from('<q', self.data, off)[0], off + 8

    def i16(self, off):
        return struct.unpack_from('<h', self.data, off)[0], off + 2

    def f32(self, off):
        return struct.unpack_from('<f', self.data, off)[0], off + 4

    def f64(self, off):
        return struct.unpack_from('<d', self.data, off)[0], off + 8

    def varint(self, off):
        v = 0; shift = 0
        while True:
            b = self.data[off]; off += 1
            v |= (b & 0x7F) << shift
            if not (b & 0x80):
                break
            shift += 7
        return v, off

    def zigzag(self, v):
        return (v >> 1) ^ -(v & 1)

    def string(self, off):
        n, off = self.varint(off)
        s = self.data[off:off + n].decode('utf-8', 'replace')
        return s, off + n

    def bytes_(self, off, n):
        return bytes(self.data[off:off + n]), off + n

    # ---- NBT(网络小端 varint) ----
    def nbt(self, off):
        def rvarint(o):
            v = 0; sh = 0
            while True:
                b = self.data[o]; o += 1
                v |= (b & 0x7F) << sh
                if not (b & 0x80): break
                sh += 7
            return v, o
        def parse(o, depth=0):
            tag = self.data[o]; o += 1
            if tag == 0:
                return None, o
            name_len, o = rvarint(o)
            name = self.data[o:o + name_len].decode('utf-8', 'replace'); o += name_len
            if tag == 1:  # byte
                val = self.data[o]; return (name, val), o + 1
            if tag == 2:  # short
                val = struct.unpack_from('<h', self.data, o)[0]; return (name, val), o + 2
            if tag == 3:  # int (zigzag varint)
                v, o = rvarint(o); return (name, self.zigzag(v)), o
            if tag == 4:  # long
                v = 0; sh = 0
                while True:
                    b = self.data[o]; o += 1
                    v |= (b & 0x7F) << sh
                    if not (b & 0x80): break
                    sh += 7
                return (name, self.zigzag(v)), o
            if tag == 5:  # float
                val = struct.unpack_from('<f', self.data, o)[0]; return (name, val), o + 4
            if tag == 6:  # double
                val = struct.unpack_from('<d', self.data, o)[0]; return (name, val), o + 8
            if tag == 7:  # byte array
                n, o = rvarint(o); return (name, list(self.data[o:o + n])), o + n
            if tag == 8:  # string
                n, o = rvarint(o); s = self.data[o:o + n].decode('utf-8', 'replace'); return (name, s), o + n
            if tag == 9:  # list
                elem = self.data[o]; o += 1
                n, o = rvarint(o)
                items = []
                for _ in range(n):
                    v, o = parse_list_elem(elem, o)
                    items.append(v)
                return (name, items), o
            if tag == 10:  # compound
                items = {}
                while True:
                    if self.data[o] == 0:
                        o += 1
                        break
                    (k, v), o = parse(o, depth + 1)
                    items[k] = v
                return (name, items), o
            if tag == 11:  # int array
                n, o = rvarint(o)
                vals = []
                for _ in range(n):
                    v, o = rvarint(o); vals.append(self.zigzag(v))
                return (name, vals), o
            if tag == 12:  # long array
                n, o = rvarint(o)
                vals = []
                for _ in range(n):
                    v = 0; sh = 0
                    while True:
                        b = self.data[o]; o += 1
                        v |= (b & 0x7F) << sh
                        if not (b & 0x80): break
                        sh += 7
                    vals.append(self.zigzag(v))
                return (name, vals), o
            raise ValueError('unknown nbt tag %d' % tag)
        def parse_list_elem(elem, o):
            # list 元素解析(无名字)
            if elem == 1:
                return self.data[o], o + 1
            if elem == 8:
                n, o = rvarint(o); return self.data[o:o + n].decode('utf-8', 'replace'), o + n
            if elem == 10:
                items = {}
                while True:
                    if self.data[o] == 0:
                        o += 1; break
                    (k, v), o = parse(o, 1)
                    items[k] = v
                return items, o
            if elem == 3:
                v, o = rvarint(o); return self.zigzag(v), o
            raise ValueError('unsupported list elem %d' % elem)
        return parse(off)[0], parse(off)[1] if False else None  # placeholder

    # ---- 主解码 ----
    def decode(self, type_name, off=0, depth=0):
        t = self.types.get(type_name)
        if t is None:
            # 原生类型
            return self._primitive(type_name, off)
        if isinstance(t, str):
            if t == 'native':
                # 类型名本身就是原语(如 zigzag32: "native")
                return self._primitive(type_name, off)
            return self.decode(t, off, depth)
        kind = t[0]
        if kind == 'container':
            fields = t[1]
            result = {}
            for f in fields:
                fname = f.get('name', '')
                v, off = self._resolve(f['type'], off, depth + 1, ctx=result)
                result[fname] = v
            return result, off
        if kind == 'array':
            spec = t[1]
            if 'count' in spec:
                n = spec['count']
            else:
                n, off = self._primitive(spec['countType'], off)
            items = []
            for _ in range(n):
                v, off = self._resolve(spec['type'], off, depth + 1)
                items.append(v)
            return items, off
        if kind == 'mapper':
            spec = t[1]
            v, off = self._primitive(spec['type'], off)
            mappings = spec.get('mappings')
            if mappings is not None:
                return mappings.get(str(v), mappings.get(v, v)), off
            return v, off
        if kind == 'switch':
            spec = t[1]
            base = spec['compareTo']  # 引用的字段名(在容器上下文中)
            # 无法独立解码 switch,需要容器传入;这里返回占位
            raise ValueError('switch cannot be decoded standalone')
        if kind == 'option':
            v, off = self._primitive('bool', off)
            val = None
            if v:
                val, off = self._resolve(t[1], off, depth + 1)
            return val, off
        if kind == 'pstring':
            spec = t[1]
            n, off = self._primitive(spec['countType'], off)
            s = self.data[off:off + n].decode('utf-8', 'replace')
            return s, off + n
        if kind == 'buffer':
            spec = t[1]
            if 'count' in spec:
                return self.bytes_(off, spec['count'])
            n, off = self.varint(off)
            return bytes(self.data[off:off + n]), off + n
        raise ValueError('unknown kind %s' % kind)

    def _resolve(self, type_spec, off, depth, ctx=None):
        if isinstance(type_spec, str):
            return self.decode(type_spec, off, depth)
        kind = type_spec[0]
        if kind in ('container', 'array', 'mapper', 'option', 'buffer', 'switch'):
            return self.decode_raw(type_spec, off, depth, ctx)
        return self._primitive(type_spec, off)

    def decode_raw(self, type_spec, off, depth, ctx=None):
        kind = type_spec[0]
        if kind == 'container':
            fields = type_spec[1]
            result = {}
            for f in fields:
                fname = f.get('name', '')
                v, off = self._resolve(f['type'], off, depth + 1, result)
                result[fname] = v
            return result, off
        if kind == 'array':
            spec = type_spec[1]
            if 'count' in spec:
                n = spec['count']
            else:
                n, off = self._primitive(spec['countType'], off)
            items = []
            for _ in range(n):
                v, off = self._resolve(spec['type'], off, depth + 1)
                items.append(v)
            return items, off
        if kind == 'mapper':
            v, off = self._primitive(type_spec[1]['type'], off)
            mappings = type_spec[1].get('mappings')
            if mappings is not None:
                return mappings.get(str(v), mappings.get(v, v)), off
            return v, off
        if kind == 'option':
            v, off = self._primitive('bool', off)
            val = None
            if v:
                val, off = self._resolve(type_spec[1], off, depth + 1)
            return val, off
        if kind == 'buffer':
            spec = type_spec[1]
            if 'count' in spec:
                return self.bytes_(off, spec['count'])
            n, off = self.varint(off)
            return bytes(self.data[off:off + n]), off + n
        if kind == 'switch':
            spec = type_spec[1]
            if ctx is None:
                raise ValueError('switch requires ctx')
            cmp_val = ctx.get(spec['compareTo'])
            fields = spec['fields']
            case = fields.get(str(cmp_val)) or fields.get(cmp_val)
            if case is None and 'true' in fields:
                case = fields.get('true' if cmp_val else 'false')
            if case is None:
                return None, off
            return self._resolve(case, off, depth + 1, ctx)
        if kind == 'pstring':
            spec = type_spec[1]
            n, off = self._primitive(spec['countType'], off)
            s = self.data[off:off + n].decode('utf-8', 'replace')
            return s, off + n
        raise ValueError('unknown kind %s' % kind)

    def _primitive(self, name, off):
        if name == 'varint': return self.varint(off)
        if name == 'zigzag32': v, off = self.varint(off); return self.zigzag(v), off
        if name == 'zigzag64': v, off = self.varint(off); return self.zigzag(v), off
        if name == 'varint64': return self.varint(off)
        if name == 'bool': return self.u8(off)
        if name == 'u8': return self.u8(off)
        if name == 'i8': return self.i8(off)
        if name == 'u16': return self.u16(off)
        if name == 'i16': return self.i16(off)
        if name == 'lu16': return self.u16(off)
        if name == 'li16': return self.i16(off)
        if name == 'lu32': return self.u32(off)
        if name == 'li32': return self.i32(off)
        if name == 'lu64': return self.u64(off)
        if name == 'li64': return self.i64(off)
        if name == 'lf32': return self.f32(off)
        if name == 'lf64': return self.f64(off)
        if name == 'string': return self.string(off)
        if name == 'vec3f':
            x, off = self.f32(off); y, off = self.f32(off); z, off = self.f32(off)
            return [x, y, z], off
        if name == 'vec2f':
            x, off = self.f32(off); y, off = self.f32(off)
            return [x, y], off
        if name == 'vec3i':
            x, off = self._primitive('zigzag32', off)
            y, off = self._primitive('zigzag32', off)
            z, off = self._primitive('zigzag32', off)
            return [x, y, z], off
        if name == 'buffer': return self.bytes_(off, 0)[0] if False else None, off  # placeholder
        if name == 'nbt': return self._nbt_top(off)
        if name == 'uuid':
            b = self.data[off:off+16]
            return b.hex(), off + 16
        if name == 'void': return None, off
        raise ValueError('unknown primitive %s' % name)

    def _nbt_top(self, off):
        # 解析一个网络 NBT compound
        def rvarint(o):
            v = 0; sh = 0
            while True:
                b = self.data[o]; o += 1
                v |= (b & 0x7F) << sh
                if not (b & 0x80): break
                sh += 7
            return v, o
        def parse(o):
            tag = self.data[o]; o += 1
            if tag == 0: return None, o
            nl, o = rvarint(o)
            name = self.data[o:o + nl].decode('utf-8', 'replace'); o += nl
            if tag == 8:
                sl, o = rvarint(o); s = self.data[o:o + sl].decode('utf-8', 'replace'); return (name, s), o + sl
            if tag == 3:
                v, o = rvarint(o); return (name, self.zigzag(v)), o
            if tag == 1: return (name, self.data[o]), o + 1
            if tag == 10:
                items = {}
                while True:
                    if self.data[o] == 0:
                        o += 1; break
                    (k, v), o = parse(o)
                    items[k] = v
                return (name, items), o
            if tag == 9:
                elem = self.data[o]; o += 1
                n, o = rvarint(o)
                items = []
                for _ in range(n):
                    # 简化 list 元素
                    if elem == 8:
                        sl, o = rvarint(o); items.append(self.data[o:o + sl].decode('utf-8', 'replace')); o += sl
                    elif elem == 3:
                        v, o = rvarint(o); items.append(self.zigzag(v))
                    else:
                        raise ValueError('nbt list elem %d' % elem)
                return (name, items), o
            raise ValueError('nbt tag %d' % tag)
        _, off = parse(off)
        return None, off


if __name__ == '__main__':
    import sys
    types = load_types('tools/minecraft-data_1.21.120_protocol.json')
    print('types loaded:', len(types))
