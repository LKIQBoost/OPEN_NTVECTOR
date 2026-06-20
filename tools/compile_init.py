# -*- coding: utf-8 -*-
import os
import sys
import struct
import marshal
import zlib
import rotor
import engine
import fop
import mod_log
import proton
import thread
import time as _time
import hashlib
import binascii
import umsgpack
import setting
import utility
from RC4 import rc4_decrypt

LOG_INFO  = 1
LOG_WARN  = 2
LOG_ERROR = 4

def _log(level, msg):
    mod_log.log(level, '[init] ' + msg)


# ════════════════════════════════════════════════════════════════
# NeoXHash (与 C++ MCPHash.h NeoXHash::StringIDLegacy 一致)
# ════════════════════════════════════════════════════════════════

def _rol32(x, n):
    x = x & 0xFFFFFFFF
    return ((x << n) | (x >> (32 - n))) & 0xFFFFFFFF

def _reduce_B(prod):
    lo = prod & 0xFFFFFFFF
    hi = (prod >> 32) & 0xFFFFFFFF
    r0 = (lo + (1 if hi else 0)) & 0xFFFFFFFF
    c0 = 1 if r0 < lo else 0
    r1 = (r0 + hi) & 0xFFFFFFFF
    c1 = 1 if r1 < hi else 0
    return (r1 + c0 + c1) & 0xFFFFFFFF

def _reduce_C(prod):
    lo = prod & 0xFFFFFFFF
    hi = (prod >> 32) & 0xFFFFFFFF
    rhi = _rol32(hi, 1)
    r = (rhi + lo) & 0xFFFFFFFF
    return (r + (2 if r < lo else 0)) & 0xFFFFFFFF

def _neox_round(A, B, C, chunk):
    A = _rol32(A, 1)
    salt = (A ^ 0x267B0B11) & 0xFFFFFFFF
    xB = (B ^ chunk) & 0xFFFFFFFF
    xC = (C ^ chunk) & 0xFFFFFFFF
    m1 = (((xC + salt) & 0xFFFFFFFF & 0xBDEB77DE) | 0x02040801) & 0xFFFFFFFF
    m2 = (((xB + salt) & 0xFFFFFFFF & 0x7D7EBBDE) | 0x00804021) & 0xFFFFFFFF
    B = _reduce_B(m1 * xB)
    C = _reduce_C(m2 * xC)
    return A, B, C

def _neox_finalize(A, B, C, xor_in, xor_out):
    A = _rol32(A, 1)
    salt = (A ^ 0x267B0B11) & 0xFFFFFFFF
    xB = (B ^ xor_in) & 0xFFFFFFFF
    xC = (C ^ xor_in) & 0xFFFFFFFF
    m1 = (((xC + salt) & 0xFFFFFFFF & 0xBDEB77DE) | 0x02040801) & 0xFFFFFFFF
    m2 = (((xB + salt) & 0xFFFFFFFF & 0x7D7EBBDE) | 0x00804021) & 0xFFFFFFFF
    B = (_reduce_B(m1 * xB) ^ xor_out) & 0xFFFFFFFF
    C = (_reduce_C(m2 * xC) ^ xor_out) & 0xFFFFFFFF
    return A, B, C

def neox_hash(s):
    if isinstance(s, unicode):
        s = s.encode('utf-8')
    length = len(s)
    if length < 1:
        return 0
    A, B, C = 0xF4FA8928, 0x37A8470E, 0x7758B42B
    i = 0
    while i + 4 <= length:
        chunk = struct.unpack_from('<I', s, i)[0]
        A, B, C = _neox_round(A, B, C, chunk)
        i += 4
    if i < length:
        tail = 0
        for j in range(length - i):
            tail |= ord(s[i + j]) << (j * 8)
        A, B, C = _neox_round(A, B, C, tail)
    A, B, C = _neox_finalize(A, B, C, 0x9BE74448, 0x66F42C48)
    A, B, C = _neox_finalize(A, B, C, 0, 0)
    return (B ^ C) & 0xFFFFFFFF


# ════════════════════════════════════════════════════════════════
# MCP 编译打包 (Program.exe --do_main mcp_compile)
# ════════════════════════════════════════════════════════════════

def _init_rotor():
    asdf_dn = 'jdh52ogodc3sk'
    asdf_dt = '=dsieq:'
    asdf_df = '|_o=2v3!"+236'
    asdf_tm = asdf_dn * 5 + (asdf_dt + asdf_dn + asdf_df) * 4 + '#' + '%' + asdf_dt * 4 + asdf_df * 6 + '*' + '@' + '-'
    return rotor.newrotor(asdf_tm)

_rot = _init_rotor()

def encode_mcs(marshal_data):
    L = list(marshal_data)
    L.reverse()
    L = [chr(ord(ch) ^ 186) for ch in L[:130]] + list(L[130:])
    data = ''.join(L)
    data = zlib.compress(data)
    data = _rot.encrypt(data)
    return data

def encode_redirect_mcs(marshal_data):
    data = zlib.compress(marshal_data)
    arr = list(data)
    if len(arr) >= 4:
        xor_bytes = struct.pack('<I', 1966019809)
        for i in range(4):
            arr[i] = chr(ord(arr[i]) ^ ord(xor_bytes[i]))
    return ''.join(arr)

def compile_py(src_path, rel_path):
    with open(src_path, 'r') as f:
        source = f.read()
    code = compile(source, rel_path, 'exec')
    return marshal.dumps(code)

MCPK_SIG = 0x4B50434D

def _to_signed32(v):
    v = v & 0xFFFFFFFF
    if v >= 0x80000000:
        return v - 0x100000000
    return v

def pack_mcp(source_dir, output_path, sign=''):
    source_dir = os.path.abspath(source_dir)
    if not os.path.isdir(source_dir):
        _log(LOG_ERROR, 'not a directory: ' + source_dir)
        return False

    py_files = []
    for root, dirs, files in os.walk(source_dir):
        for fn in files:
            if fn.endswith('.py'):
                full = os.path.join(root, fn)
                rel = os.path.relpath(full, source_dir).replace('\\', '/')
                py_files.append((rel, full))

    if not py_files:
        _log(LOG_ERROR, 'no .py files in ' + source_dir)
        return False

    has_redirect = any(rel == 'redirect.py' for rel, _ in py_files)
    if not has_redirect:
        _log(LOG_ERROR, 'redirect.py not found (required)')
        return False

    encoded = {}
    _log(LOG_INFO, 'compiling %d files...' % len(py_files))

    for rel_py, full_path in py_files:
        mcs_rel = rel_py[:-3] + '.mcs'
        try:
            marshal_data = compile_py(full_path, rel_py)
        except Exception as e:
            _log(LOG_ERROR, 'COMPILE FAILED: %s - %s' % (rel_py, str(e)))
            if rel_py == 'redirect.py':
                return False
            continue

        if rel_py == 'redirect.py':
            enc_data = encode_redirect_mcs(marshal_data)
        else:
            enc_data = encode_mcs(marshal_data)

        encoded[mcs_rel] = (enc_data, len(marshal_data))
        _log(LOG_INFO, '  %s -> %s (%d -> %d bytes)' % (
            rel_py, mcs_rel, len(marshal_data), len(enc_data)))

    if not encoded:
        _log(LOG_ERROR, 'no files compiled successfully')
        return False

    dir_groups = {}
    for mcs_path, (data, origin_len) in encoded.items():
        slash = mcs_path.rfind('/')
        if slash >= 0:
            dir_path = mcs_path[:slash]
            filename = mcs_path[slash + 1:]
        else:
            dir_path = ''
            filename = mcs_path
        if dir_path not in dir_groups:
            dir_groups[dir_path] = []
        dir_groups[dir_path].append((filename, data, origin_len))

    dir_list = []
    for dir_path in dir_groups:
        if dir_path:
            dh = _to_signed32(neox_hash(dir_path))
        else:
            dh = 0
        dir_list.append((dh, dir_path))
    dir_list.sort(key=lambda x: x[0])

    all_dir_entries = []
    all_file_entries = []
    stream_parts = []
    fe_offset = 0
    stream_offset = 0

    for dir_hash, dir_path in dir_list:
        group = dir_groups[dir_path]
        file_items = []
        for filename, data, origin_len in group:
            fh = _to_signed32(neox_hash(filename))
            file_items.append((fh, data, origin_len))
        file_items.sort(key=lambda x: x[0])

        all_dir_entries.append((dir_hash, fe_offset, len(file_items)))

        for fh, data, origin_len in file_items:
            all_file_entries.append((fh, stream_offset, len(data), origin_len))
            stream_parts.append(data)
            stream_offset += len(data)

        fe_offset += len(file_items) * 16

    sign_data = sign if sign else ''
    sign_size = len(sign_data)

    header_size = 24
    dir_table_size = len(all_dir_entries) * 12
    file_table_size = len(all_file_entries) * 16

    dir_table_off = header_size + sign_size
    file_table_off = dir_table_off + dir_table_size
    stream_off = file_table_off + file_table_size

    now = _time.time()

    f = open(output_path, 'wb')
    f.write(struct.pack('<I', MCPK_SIG))
    f.write(struct.pack('<d', now))
    f.write(struct.pack('<I', dir_table_off))
    f.write(struct.pack('<I', file_table_off))
    f.write(struct.pack('<I', stream_off))

    if sign_data:
        f.write(sign_data)

    for dir_id, offset, count in all_dir_entries:
        f.write(struct.pack('<iII', dir_id, offset, count))

    for file_id, offset, length, origin_len in all_file_entries:
        f.write(struct.pack('<iIII', file_id, offset, length, origin_len))

    for part in stream_parts:
        f.write(part)

    f.close()

    total_size = stream_off + stream_offset
    _log(LOG_INFO, 'output: %s (%d files, %d dirs, %d bytes)' % (
        output_path, len(all_file_entries), len(all_dir_entries), total_size))
    if sign_data:
        _log(LOG_INFO, 'sign: %s' % sign)
    return True

def _do_mcp_compile(args):
    params = engine.getparams()
    source_dir = None
    output_path = None
    sign = ''

    if isinstance(params, (list, tuple)):
        try:
            idx = params.index('mcp_compile')
            rest = params[idx + 1:]
        except ValueError:
            rest = []

        positional = []
        i = 0
        while i < len(rest):
            if rest[i] == '--sign' and i + 1 < len(rest):
                sign = rest[i + 1]
                i += 2
            elif rest[i] == '--sign_file' and i + 1 < len(rest):
                with open(rest[i + 1], 'rb') as sf:
                    sign = sf.read()
                i += 2
            else:
                positional.append(rest[i])
                i += 1

        if len(positional) >= 2:
            source_dir = positional[0]
            output_path = positional[1]
        elif len(positional) == 1:
            source_dir = positional[0]

    if not source_dir:
        source_dir = 'source'
    if not output_path:
        output_path = 'vanilla.mcp'

    _log(LOG_INFO, 'mcp_compile: %s -> %s' % (source_dir, output_path))
    ok = pack_mcp(source_dir, output_path, sign)
    sys.exit(0 if ok else 1)

engine.register(_do_mcp_compile, "mcp_compile")


# ════════════════════════════════════════════════════════════════
# 插件管理
#
# 插件生命周期:
#   加载:  start_game -> load_all_plugins -> 各插件 import/register
#   运行:  插件通过 engine.register() 注册用户事件
#   单插件重载: 调 on_uninit() -> 注销该插件事件 -> reload 该插件
#   全量重载:   ModEventUnInit -> cleanup_user -> reload 所有插件
#
# 插件开发规范:
#   - 注册事件只用 engine.register()（用户事件），重载时自动清理
#   - engine.register_kernel_event() 是 init.py 专用，插件不要用
#   - 如果开了线程，实现 on_uninit() 函数并在其中停止线程
#   - 全量重载: ModEventUnInit -> 所有用户事件清空 -> 插件重新执行
#   - 单插件重载: on_uninit() -> 该插件事件精准注销 -> 该插件重新执行
# ════════════════════════════════════════════════════════════════

SCRIPTS_DIR = os.path.abspath('./scripts')

_loaded_plugins = []
_file_mtimes = {}
_watching = False


def _call_plugin_uninit(name):
    mod = sys.modules.get(name)
    if mod and hasattr(mod, 'on_uninit'):
        try:
            mod.on_uninit()
        except Exception:
            import traceback
            _log(LOG_ERROR, 'on_uninit failed: %s' % name)
            traceback.print_exc()


def _file_to_plugin(filepath):
    rel = os.path.relpath(filepath, SCRIPTS_DIR).replace('\\', '/')
    if '/' in rel:
        return rel.split('/')[0]
    if rel.endswith('.py'):
        return rel[:-3]
    if rel.endswith('.mcp'):
        return rel[:-4]
    return None


def _load_plugin_redirect(plugin_root):
    redirect_path = os.path.join(plugin_root, 'redirect.py')
    if os.path.exists(redirect_path):
        try:
            execfile(redirect_path, {'__name__': '__redirect__', '__file__': redirect_path})
        except Exception:
            import traceback
            _log(LOG_WARN, 'redirect.py exec failed: %s' % plugin_root)
            traceback.print_exc()


def load_plugin(name, is_mcp=False, mcp_path=None):
    plugin_root = os.path.join(SCRIPTS_DIR, name)

    if is_mcp and mcp_path:
        fop.new_mcp(mcp_path)
    else:
        if os.path.isdir(plugin_root) and plugin_root not in sys.path:
            sys.path.insert(0, plugin_root)
            _load_plugin_redirect(plugin_root)

    try:
        if name in sys.modules:
            reload(sys.modules[name])
        else:
            __import__(name)
        if name not in _loaded_plugins:
            _loaded_plugins.append(name)
        _log(LOG_INFO, 'plugin loaded: %s' % name)
        return True
    except Exception as e:
        _log(LOG_ERROR, 'plugin load FAILED: %s - %s' % (name, str(e)))
        import traceback
        traceback.print_exc()
        return False


def unload_plugin(name):
    _call_plugin_uninit(name)

    events_info = engine.get_events_info()
    for event_name, callbacks in events_info.items():
        for cb in callbacks:
            if getattr(cb, '__module__', None) == name:
                engine.unregister(event_name, cb)

    if name in _loaded_plugins:
        _loaded_plugins.remove(name)
    if name in sys.modules:
        del sys.modules[name]
    _log(LOG_INFO, 'plugin unloaded: %s' % name)


def load_all_plugins():
    if not os.path.exists(SCRIPTS_DIR):
        _log(LOG_WARN, 'scripts/ not found, skipping')
        return

    use_mcp = engine.get_mcp_load_config()

    for item in sorted(os.listdir(SCRIPTS_DIR)):
        item_path = os.path.join(SCRIPTS_DIR, item)
        if use_mcp:
            if item.endswith('.mcp'):
                load_plugin(item[:-4], True, item_path)
        else:
            if os.path.isdir(item_path):
                inner_init = os.path.join(item_path, item, '__init__.py')
                if os.path.exists(inner_init):
                    load_plugin(item)


def reload_all_plugins():
    _log(LOG_INFO, 'reloading all plugins...')

    engine.trigger("ModEventUnInit")

    engine.cleanup_user()
    engine.clear_all_user_protocol_event()

    engine.register(_do_mcp_compile, "mcp_compile")

    use_mcp = engine.get_mcp_load_config()

    for name in list(_loaded_plugins):
        if use_mcp:
            mcp_path = os.path.join(SCRIPTS_DIR, name + '.mcp')
            if os.path.exists(mcp_path):
                fop.reload_mcp(mcp_path)

        if name in sys.modules:
            try:
                reload(sys.modules[name])
                _log(LOG_INFO, 'plugin reloaded: %s' % name)
            except Exception as e:
                _log(LOG_ERROR, 'plugin reload FAILED: %s - %s' % (name, str(e)))
                import traceback
                traceback.print_exc()
        else:
            try:
                __import__(name)
                _log(LOG_INFO, 'plugin reloaded (import): %s' % name)
            except Exception as e:
                _log(LOG_ERROR, 'plugin reload FAILED: %s - %s' % (name, str(e)))

    engine.trigger("ModEventStartUp")
    _log(LOG_INFO, 'reload complete (%d plugins)' % len(_loaded_plugins))


def reload_plugin(name):
    if name not in _loaded_plugins:
        _log(LOG_WARN, 'plugin not loaded: %s' % name)
        return False

    _call_plugin_uninit(name)

    events_info = engine.get_events_info()
    for event_name, callbacks in events_info.items():
        for cb in callbacks:
            if getattr(cb, '__module__', None) == name:
                engine.unregister(event_name, cb)

    use_mcp = engine.get_mcp_load_config()
    if use_mcp:
        mcp_path = os.path.join(SCRIPTS_DIR, name + '.mcp')
        if os.path.exists(mcp_path):
            fop.reload_mcp(mcp_path)

    if name in sys.modules:
        try:
            reload(sys.modules[name])
            _log(LOG_INFO, 'plugin reloaded: %s' % name)
            return True
        except Exception as e:
            _log(LOG_ERROR, 'plugin reload FAILED: %s - %s' % (name, str(e)))
            import traceback
            traceback.print_exc()
            return False
    return False


# ════════════════════════════════════════════════════════════════
# 文件监视 (开发热重载，精准到单插件)
# ════════════════════════════════════════════════════════════════

def _scan_changes():
    changed_plugins = set()
    if not os.path.exists(SCRIPTS_DIR):
        return changed_plugins
    for root, dirs, files in os.walk(SCRIPTS_DIR):
        for fn in files:
            if fn.endswith('.py') or fn.endswith('.mcp'):
                fp = os.path.join(root, fn)
                try:
                    mt = os.path.getmtime(fp)
                except OSError:
                    continue
                old = _file_mtimes.get(fp)
                if old is not None and mt != old:
                    plugin_name = _file_to_plugin(fp)
                    if plugin_name and plugin_name in _loaded_plugins:
                        changed_plugins.add(plugin_name)
                    _log(LOG_INFO, 'file changed: %s' % os.path.relpath(fp, SCRIPTS_DIR))
                _file_mtimes[fp] = mt
    return changed_plugins


def _watch_loop():
    global _watching
    _scan_changes()
    while _watching:
        _time.sleep(1.5)
        try:
            changed = _scan_changes()
            for name in changed:
                reload_plugin(name)
        except Exception:
            import traceback
            traceback.print_exc()


def start_watch():
    global _watching
    if _watching:
        return
    _watching = True
    thread.start_new_thread(_watch_loop, ())
    _log(LOG_INFO, 'file watcher started')


def stop_watch():
    global _watching
    _watching = False
    _log(LOG_INFO, 'file watcher stopped')


# ════════════════════════════════════════════════════════════════
# MCP 校验 (服务端反作弊)
# ════════════════════════════════════════════════════════════════

def _mcp_init_rotor():
    asdf_dn = 'j2h56ogodh3sk'
    asdf_dt = '=dziaq;'
    asdf_df = '|`o=5v7!"-276'
    asdf_tm = asdf_dn * 4 + (asdf_dt + asdf_dn + asdf_df) * 5 + '$' + '@' + asdf_dt * 7 + asdf_df * 2 + '&' + ']' + '`'
    return rotor.newrotor(asdf_tm)

def _mcp_reverse_data(data):
    L = list(data)
    L = map((lambda ch: chr(ord(ch) ^ 156)), L[:130]) + L[130:]
    L.reverse()
    return ''.join(L)

def _mcp_rotor_decrypt(mcp):
    rot = _mcp_init_rotor()
    mcp = rot.decrypt(binascii.unhexlify(mcp))
    mcp = zlib.decompress(mcp)
    mcp = _mcp_reverse_data(mcp)
    return mcp

def _get_calc_check_num(mcp):
    mcp = _mcp_rotor_decrypt(mcp)
    key = binascii.unhexlify("8D06E8C8B7D7B7284651AE04")
    offset = 141
    size = ord(mcp[offset]) + 4
    str1 = rc4_decrypt(key, mcp[offset + 4:offset + size])
    offset += size + 1
    size = ord(mcp[offset]) + 4
    str2 = rc4_decrypt(key, mcp[offset + 4:offset + size])
    def md5(salt):
        return hashlib.md5(str1 + salt + str2).hexdigest()
    return md5

def _mcp_check_num(mcp, salt, player_id, engine_version, patch_version):
    calc = _get_calc_check_num(mcp)
    valM = calc(salt + "0")
    raw_params = ['False', '[]', '', '', '3']
    params = [False, [], '', '', 3]
    uid_int = int(setting.get_uid())
    if uid_int > 2147483647:
        tmps = ([(ord(c) * 2 + 5) ^ 255 for c in valM] +
                [engine_version, "android", patch_version, "android", 2, 12, player_id])
    else:
        tmps = ([(ord(c) * 2 + 5) ^ 255 for c in valM] +
                [engine_version, "windows", engine_version, "win32", 0, 12, player_id])
    game_info = calc(''.join([str(i) for i in tmps]))
    raw_params.append(game_info)
    params.append(game_info)
    raw = valM[16:] + ''.join(raw_params) + valM[:16]
    sign = calc(raw)
    return ["SetMCPCheckNum", [[valM, sign] + params], None]


# ════════════════════════════════════════════════════════════════
# Kernel 事件（init.py 专用，reload 不清除）
# ════════════════════════════════════════════════════════════════

def _on_rpc(args):
    resp = umsgpack.unpackb(args[0])
    cmd = resp[0]
    if cmd == "S2CHeartBeat":
        engine.rpc(umsgpack.packb(['ClientLoadAddonsFinishedFromGac', [], None]))
    elif cmd == "GetStartType":
        data = utility.decrypt_with_tail(binascii.unhexlify(resp[1][0]))
        uid = setting.get_uid()
        resp[1][0] = binascii.hexlify(utility.encrypt_with_tail(uid + data))
        resp[0] = "SetStartType"
        engine.rpc(umsgpack.packb(resp))
    elif cmd == "GetMCPCheckNum":
        engine.rpc(umsgpack.packb(['C2SHeartBeat',
            [{'is_64': False, 't': 1748429465, 'is_android': False}], None]))
        rpc = _mcp_check_num(resp[1][0], resp[1][1][0],
            setting.get_playerid(), setting.get_engine_version(),
            setting.get_patch_version())
        engine.rpc(umsgpack.packb(rpc))
    elif cmd == "c":
        engine.trigger("uid2id", resp[1][1])

def _on_start_game(args):
    engine.rpc(umsgpack.packb(['e', [], None]))
    engine.rpc(umsgpack.packb(['SyncUsingMod',
        [[], 'c18e65aa-7b21-4637-9b63-8ad63622ef01', None, False, {}], None]))
    proton.init()
    load_all_plugins()
    start_watch()
    engine.trigger("ModEventStartUp")

def _on_reload(args):
    reload_all_plugins()

engine.register_kernel_event(_on_rpc, "on_rpc")
engine.register_kernel_event(_on_start_game, "start_game")
engine.register_kernel_event(_on_reload, "reload")
engine.register_kernel_event(_on_reload, "mod_reload")
