# -*- coding: utf-8 -*-
import pkt
import engine
import mod_log

LOG_INFO  = 1
LOG_WARN  = 2
LOG_ERROR = 4

def _log(level, msg):
    mod_log.log(level, '[proton] ' + msg)

# ════════════════════════════════════════════════════════════════
# Bedrock Protocol -> Engine Event 转换层
#
# C++ 层通过 engine.register_protocol_event(packet_id, cb, is_kernel)
# 将原始二进制数据包交给 Python，这里解析后触发高层事件给插件。
#
# 使用 kernel protocol event，reload 不清除。
# 插件通过 engine.register("OnTextPacket", handler) 接收解析后的数据。
# ════════════════════════════════════════════════════════════════

# TextPacket (0x09) message types
TEXT_RAW           = 0
TEXT_CHAT          = 1
TEXT_TRANSLATION   = 2
TEXT_POPUP         = 3
TEXT_JUKEBOX       = 4
TEXT_TIP           = 5
TEXT_SYSTEM        = 6
TEXT_WHISPER        = 7
TEXT_ANNOUNCEMENT  = 8
TEXT_OBJECT        = 9
TEXT_OBJECT_WHISPER = 10


def _on_text_packet(data):
    try:
        off = 0
        msg_type, needs_translation, off = pkt.unpack('b?', data, off)

        source_name = ''
        message = ''
        parameters = []
        xuid = ''
        platform_chat_id = ''
        filtered_message = ''

        if msg_type in (TEXT_CHAT, TEXT_WHISPER, TEXT_ANNOUNCEMENT):
            source_name, message, off = pkt.unpack('ss', data, off)
        elif msg_type in (TEXT_RAW, TEXT_TIP, TEXT_SYSTEM):
            message, off = pkt.unpack('s', data, off)
        elif msg_type in (TEXT_TRANSLATION, TEXT_POPUP, TEXT_JUKEBOX):
            message, off = pkt.unpack('s', data, off)
            param_count, off = pkt.unpack('v', data, off)
            for _ in range(param_count):
                p, off = pkt.read_string(data, off)
                parameters.append(p)

        if pkt.remaining(data, off) > 0:
            xuid, off = pkt.unpack('s', data, off)
        if pkt.remaining(data, off) > 0:
            platform_chat_id, off = pkt.unpack('s', data, off)
        if pkt.remaining(data, off) > 0:
            filtered_message, off = pkt.unpack('s', data, off)

        event_data = {
            'type': msg_type,
            'needs_translation': needs_translation,
            'source_name': source_name,
            'message': message,
            'parameters': parameters,
            'xuid': xuid,
            'platform_chat_id': platform_chat_id,
            'filtered_message': filtered_message,
        }

        engine.trigger("OnTextPacket", [event_data])

    except Exception:
        import traceback
        _log(LOG_ERROR, 'TextPacket parse failed')
        traceback.print_exc()


def init():
    engine.register_protocol_event(9, _on_text_packet, True)
    _log(LOG_INFO, 'protocol handlers registered')
