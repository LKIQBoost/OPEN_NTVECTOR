#pragma once
#include <Python.h>
#include <string>
#include <vector>
#include <initializer_list>
#include <array>
#include "Logger.h"
#include "CommandOutput.h"
#include "PlayerList.h"
#include "Respawn.h"
#include "Py3Compat.h"

extern "C" {
    void trigger_event(const char* event_name, PyObject* args_array);
    void trigger_event_with_args(const char* event_name, int arg_count, ...);
}

class PythonEventEngine {
private:

public:
    // ���캯��
    PythonEventEngine() {
    }

    // �����޲����¼�
    void triggerEvent(const std::string& event_name) {
        Logger::getInstance().log(LOG_INFO, std::string() + "[Engine] Event Trigger: " + event_name);
        PyGILState_STATE gstate = PyGILState_Ensure();

        PyObject* empty_list = PyList_New(0);
        trigger_event(event_name.c_str(), empty_list);
        Py_DECREF(empty_list);

        PyGILState_Release(gstate);
    }

    // ������Python����������¼�
    void triggerEvent(const std::string& event_name, PyObject* args_array) {
        Logger::getInstance().log(LOG_INFO, std::string() + "[Engine] Event Trigger: " + event_name);
        PyGILState_STATE gstate = PyGILState_Ensure();
        trigger_event(event_name.c_str(), args_array);
        PyGILState_Release(gstate);
    }

    // ���������Python����������¼�
    void triggerEvent(const std::string& event_name, const std::vector<PyObject*>& args) {
        Logger::getInstance().log(LOG_INFO, std::string() + "[Engine] Event Trigger: " + event_name);
        PyGILState_STATE gstate = PyGILState_Ensure();

        PyObject* args_list = PyList_New(args.size());
        for (size_t i = 0; i < args.size(); i++) {
            Py_INCREF(args[i]); // �������ü���
            PyList_SetItem(args_list, i, args[i]);
        }

        trigger_event(event_name.c_str(), args_list);
        Py_DECREF(args_list);

        PyGILState_Release(gstate);
    }

    // ��������ʼֵ�б��������¼�
    void triggerEvent(const std::string& event_name, std::initializer_list<PyObject*> args) {
        Logger::getInstance().log(LOG_INFO, std::string() + "[Engine] Event Trigger: " + event_name);
        PyGILState_STATE gstate = PyGILState_Ensure();

        PyObject* args_list = PyList_New(args.size());
        size_t i = 0;
        for (auto arg : args) {
            Py_INCREF(arg); // �������ü���
            PyList_SetItem(args_list, i++, arg);
        }

        trigger_event(event_name.c_str(), args_list);
        Py_DECREF(args_list);

       PyGILState_Release(gstate);
    }

    // ģ�庯�����Զ�ת��C++����ΪPython����
    template<typename... Args>
    void trigger(const std::string& event_name, Args&&... args) {
        Logger::getInstance().log(LOG_INFO, std::string() + "[Engine] Event Trigger: " + event_name);
        PyGILState_STATE gstate = PyGILState_Ensure();

        PyObject* args_list = PyList_New(sizeof...(Args));
        if (!args_list) {
            PyErr_Print();
            Logger::getInstance().log(LOG_ERROR, "[Engine] Failed to create Python args list");
            PyGILState_Release(gstate);
            return;
        }

        addArgumentsToList(args_list, 0, std::forward<Args>(args)...);

        trigger_event(event_name.c_str(), args_list);
        Py_DECREF(args_list);

        PyGILState_Release(gstate);
    }


    void trigger(const std::string& event_name, PyObject* args) {
        trigger_event(event_name.c_str(), args);
    }

    // Trigger an event whose payload is raw binary (rpc data, packet bytes).
    // In Py3 the std::string overload would decode it as UTF-8 into str, so we
    // deliver it as bytes explicitly. trigger_event REQUIRES a list/tuple
    // argument (it silently drops non-list args), so wrap the bytes in a list.
    void triggerBytes(const std::string& event_name, const std::string& binary) {
        PyGILState_STATE gstate = PyGILState_Ensure();
        PyObject* b = PyBytes_FromStringAndSize(binary.data(), (Py_ssize_t)binary.size());
        if (b) {
            PyObject* args = PyList_New(1);
            PyList_SetItem(args, 0, b);   // steals the reference to b
            trigger_event(event_name.c_str(), args);
            Py_DECREF(args);
        }
        PyGILState_Release(gstate);
    }

private:
    // �ݹ���ֹ����
    void addArgumentsToList(PyObject* list, int index) {
        // �ݹ���ֹ
    }

    // �ݹ����Ӳ������б�
    template<typename T, typename... Rest>
    void addArgumentsToList(PyObject* list, int index, T&& value, Rest&&... rest) {
        PyObject* py_obj = convertToPythonObject(std::forward<T>(value));
        PyList_SetItem(list, index, py_obj);
        addArgumentsToList(list, index + 1, std::forward<Rest>(rest)...);
    }

    // ����ת������
    PyObject* convertToPythonObject(int value) {
        return PyLong_FromLong(value);
    }

    PyObject* convertToPythonObject(long value) {
        return PyLong_FromLong(value);
    }

    PyObject* convertToPythonObject(uint64_t value) {
        return PyLong_FromLongLong(value);
    }

    PyObject* convertToPythonObject(double value) {
        return PyFloat_FromDouble(value);
    }
    PyObject* convertToPythonObject(std::vector<std::string>& value) {

        // 1. ����һ���յ�Python2.7 list������������Ҫ���صĶ���
        PyObject* py_list = PyList_New(0);
        if (py_list == NULL) {
            // ����ʧ�ܣ��׳�Python�쳣���ڴ治��ȣ������ؿ�ָ��
            PyErr_SetString(PyExc_MemoryError, "Failed to create Python list");
            return NULL;
        }

        // 2. ����C++��std::vector<std::string>���������Ԫ��ת��+���ӵ�Python list
        for (std::vector<std::string>::const_iterator it = value.begin();
            it != value.end(); ++it)
        {
            // 2.1 ��C++ std::string ת Python2.7 str����
            // PyString_FromString ��Python2.7ר��API��ר��ת const char* -> Python str
            PyObject* py_str = PyTextFromUtf8(it->c_str());
            if (py_str == NULL) {
                // �ַ���ת��ʧ�ܣ��ͷ��Ѵ�����Python���󣬷�ֹ�ڴ�й©
                Py_DECREF(py_list);
                PyErr_SetString(PyExc_TypeError, "Failed to convert C++ string to Python str");
                return NULL;
            }

            // 2.2 ��ת�����Python str ���ӵ�Python list��ĩβ
            // PyList_Append ��Python2.7 API����listβ��׷��Ԫ��
            int ret = PyList_Append(py_list, py_str);
            if (ret != 0) {
                // ׷��ʧ�ܣ��ͷ������Ѵ�������
                Py_DECREF(py_str);
                Py_DECREF(py_list);
                PyErr_SetString(PyExc_RuntimeError, "Failed to append element to Python list");
                return NULL;
            }

            // 2.3 �ؼ����ͷ�Python�ַ��������ü����������ڴ�й©
            // Python2.7�����ü������ƣ�py_str�Ѿ���list���У������ֶ���1
            Py_DECREF(py_str);
        }

        // 3. ת����ɣ�����Python list�����ⲿ��ֱ����Python�����Ϊԭ��list��
        return py_list;
    }

    PyObject* convertToPythonObject(const char* value) {
        return PyTextFromUtf8(value);
    }

    PyObject* convertToPythonObject(const std::string& value) {
        return PyTextFromUtf8(value.c_str(), (Py_ssize_t)value.size());
    }

    PyObject* convertToPythonObject(const std::vector<uint8_t>& value) {
        return PyBytes_FromStringAndSize((const char*)value.data(), (Py_ssize_t)value.size());
    }

    PyObject* convertToPythonObject(bool value) {
        return PyBool_FromLong(value ? 1 : 0);
    }

    // ����Python����ֱ�Ӵ��ݣ�
    PyObject* convertToPythonObject(PyObject* value) {
        Py_INCREF(value);
        return value;
    }

    // ===== CommandOutput 相关类型转换 =====

    // 将 UUID (16字节数组) 转换为 Python 字符串 (十六进制格式)
    PyObject* convertUUIDToPythonString(const std::array<uint8_t, 16>& uuid) {
        char hex_str[37]; // 32 hex chars + 4 dashes + null terminator
        snprintf(hex_str, sizeof(hex_str),
            "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
            uuid[0], uuid[1], uuid[2], uuid[3],
            uuid[4], uuid[5],
            uuid[6], uuid[7],
            uuid[8], uuid[9],
            uuid[10], uuid[11], uuid[12], uuid[13], uuid[14], uuid[15]);
        return PyTextFromUtf8(hex_str);
    }

    // 将 CommandOrigin 转换为 Python 字典
    PyObject* convertToPythonObject(const CommandOrigin& origin) {
        PyObject* dict = PyDict_New();
        if (dict == NULL) {
            PyErr_SetString(PyExc_MemoryError, "Failed to create Python dict for CommandOrigin");
            return NULL;
        }

        // Origin 类型
        PyObject* py_origin = PyLong_FromLong(origin.Origin);
        PyDict_SetItemString(dict, "origin", py_origin);
        Py_DECREF(py_origin);

        // UUID
        PyObject* py_uuid = convertUUIDToPythonString(origin.UUID);
        PyDict_SetItemString(dict, "uuid", py_uuid);
        Py_DECREF(py_uuid);

        // RequestID
        PyObject* py_request_id = PyTextFromUtf8(origin.RequestID.c_str());
        PyDict_SetItemString(dict, "request_id", py_request_id);
        Py_DECREF(py_request_id);

        // PlayerUniqueID
        PyObject* py_player_id = PyLong_FromLongLong(origin.PlayerUniqueID);
        PyDict_SetItemString(dict, "player_unique_id", py_player_id);
        Py_DECREF(py_player_id);

        return dict;
    }

    // 将 CommandOutputMessage 转换为 Python 字典
    PyObject* convertToPythonObject(const CommandOutputMessage& msg) {
        PyObject* dict = PyDict_New();
        if (dict == NULL) {
            PyErr_SetString(PyExc_MemoryError, "Failed to create Python dict for CommandOutputMessage");
            return NULL;
        }

        // Success
        PyObject* py_success = PyBool_FromLong(msg.Success ? 1 : 0);
        PyDict_SetItemString(dict, "success", py_success);
        Py_DECREF(py_success);

        // Message
        PyObject* py_message = PyTextFromUtf8(msg.Message.c_str());
        PyDict_SetItemString(dict, "message", py_message);
        Py_DECREF(py_message);

        // Parameters (字符串列表)
        PyObject* py_params = PyList_New(msg.Parameters.size());
        for (size_t i = 0; i < msg.Parameters.size(); i++) {
            PyObject* py_param = PyTextFromUtf8(msg.Parameters[i].c_str());
            PyList_SetItem(py_params, i, py_param); // SetItem steals reference
        }
        PyDict_SetItemString(dict, "parameters", py_params);
        Py_DECREF(py_params);

        return dict;
    }

    // 将 std::vector<CommandOutputMessage> 转换为 Python 列表
    PyObject* convertToPythonObject(const std::vector<CommandOutputMessage>& messages) {
        PyObject* py_list = PyList_New(messages.size());
        if (py_list == NULL) {
            PyErr_SetString(PyExc_MemoryError, "Failed to create Python list for CommandOutputMessages");
            return NULL;
        }

        for (size_t i = 0; i < messages.size(); i++) {
            PyObject* py_msg = convertToPythonObject(messages[i]);
            PyList_SetItem(py_list, i, py_msg); // SetItem steals reference
        }

        return py_list;
    }

    // 将 CommandOutput 转换为 Python 字典
    PyObject* convertToPythonObject(const CommandOutput& output) {
        PyObject* dict = PyDict_New();
        if (dict == NULL) {
            PyErr_SetString(PyExc_MemoryError, "Failed to create Python dict for CommandOutput");
            return NULL;
        }

        // Origin (命令来源)
        PyObject* py_origin = convertToPythonObject(output.Origin);
        PyDict_SetItemString(dict, "origin", py_origin);
        Py_DECREF(py_origin);

        // OutputType
        PyObject* py_output_type = PyLong_FromLong(output.OutputType);
        PyDict_SetItemString(dict, "output_type", py_output_type);
        Py_DECREF(py_output_type);

        // SuccessCount
        PyObject* py_success_count = PyLong_FromLong(output.SuccessCount);
        PyDict_SetItemString(dict, "success_count", py_success_count);
        Py_DECREF(py_success_count);

        // OutputMessages
        PyObject* py_messages = convertToPythonObject(output.OutputMessages);
        PyDict_SetItemString(dict, "output_messages", py_messages);
        Py_DECREF(py_messages);

        // DataSet (仅当 OutputType == 4 时有值)
        PyObject* py_dataset = PyTextFromUtf8(output.DataSet.c_str());
        PyDict_SetItemString(dict, "data_set", py_dataset);
        Py_DECREF(py_dataset);

        return dict;
    }

    // ===== PlayerList 相关类型转换 =====

    // 将 PlayerSkin 转换为 Python 字典 (简化版，包含主要字段)
    PyObject* convertToPythonObject(const PlayerSkin& skin) {
        PyObject* dict = PyDict_New();
        if (dict == NULL) {
            PyErr_SetString(PyExc_MemoryError, "Failed to create Python dict for PlayerSkin");
            return NULL;
        }

        // SkinID
        PyObject* py_skin_id = PyTextFromUtf8(skin.SkinID.c_str());
        PyDict_SetItemString(dict, "skin_id", py_skin_id);
        Py_DECREF(py_skin_id);

        // PlayFabID
        PyObject* py_playfab_id = PyTextFromUtf8(skin.PlayFabID.c_str());
        PyDict_SetItemString(dict, "playfab_id", py_playfab_id);
        Py_DECREF(py_playfab_id);

        // 皮肤图像尺寸
        PyObject* py_skin_width = PyLong_FromLong(skin.SkinImageWidth);
        PyDict_SetItemString(dict, "skin_image_width", py_skin_width);
        Py_DECREF(py_skin_width);

        PyObject* py_skin_height = PyLong_FromLong(skin.SkinImageHeight);
        PyDict_SetItemString(dict, "skin_image_height", py_skin_height);
        Py_DECREF(py_skin_height);

        // 披风图像尺寸
        PyObject* py_cape_width = PyLong_FromLong(skin.CapeImageWidth);
        PyDict_SetItemString(dict, "cape_image_width", py_cape_width);
        Py_DECREF(py_cape_width);

        PyObject* py_cape_height = PyLong_FromLong(skin.CapeImageHeight);
        PyDict_SetItemString(dict, "cape_image_height", py_cape_height);
        Py_DECREF(py_cape_height);

        // CapeID
        PyObject* py_cape_id = PyTextFromUtf8(skin.CapeID.c_str());
        PyDict_SetItemString(dict, "cape_id", py_cape_id);
        Py_DECREF(py_cape_id);

        // FullID
        PyObject* py_full_id = PyTextFromUtf8(skin.FullID.c_str());
        PyDict_SetItemString(dict, "full_id", py_full_id);
        Py_DECREF(py_full_id);

        // ArmSize
        PyObject* py_arm_size = PyTextFromUtf8(skin.ArmSize.c_str());
        PyDict_SetItemString(dict, "arm_size", py_arm_size);
        Py_DECREF(py_arm_size);

        // SkinColour
        PyObject* py_skin_colour = PyTextFromUtf8(skin.SkinColour.c_str());
        PyDict_SetItemString(dict, "skin_colour", py_skin_colour);
        Py_DECREF(py_skin_colour);

        // 布尔标志
        PyObject* py_premium = PyBool_FromLong(skin.PremiumSkin ? 1 : 0);
        PyDict_SetItemString(dict, "premium_skin", py_premium);
        Py_DECREF(py_premium);

        PyObject* py_persona = PyBool_FromLong(skin.PersonaSkin ? 1 : 0);
        PyDict_SetItemString(dict, "persona_skin", py_persona);
        Py_DECREF(py_persona);

        PyObject* py_trusted = PyBool_FromLong(skin.Trusted ? 1 : 0);
        PyDict_SetItemString(dict, "trusted", py_trusted);
        Py_DECREF(py_trusted);

        // 动画数量 (网易版直接存储数量)
        PyObject* py_anim_count = PyLong_FromLong(skin.AnimationCount);
        PyDict_SetItemString(dict, "animation_count", py_anim_count);
        Py_DECREF(py_anim_count);

        // SkinResourcePatch
        PyObject* py_resource_patch = PyTextFromUtf8(skin.SkinResourcePatch.c_str());
        PyDict_SetItemString(dict, "skin_resource_patch", py_resource_patch);
        Py_DECREF(py_resource_patch);

        // Persona 部件列表 (网易版是字符串列表)
        PyObject* py_pieces = PyList_New(skin.PersonaPieces.size());
        for (size_t i = 0; i < skin.PersonaPieces.size(); i++) {
            PyObject* py_piece = PyTextFromUtf8(skin.PersonaPieces[i].c_str());
            PyList_SetItem(py_pieces, i, py_piece);
        }
        PyDict_SetItemString(dict, "persona_pieces", py_pieces);
        Py_DECREF(py_pieces);

        // Persona 颜色列表
        PyObject* py_tints = PyList_New(skin.PieceTintColours.size());
        for (size_t i = 0; i < skin.PieceTintColours.size(); i++) {
            PyObject* py_tint = PyTextFromUtf8(skin.PieceTintColours[i].c_str());
            PyList_SetItem(py_tints, i, py_tint);
        }
        PyDict_SetItemString(dict, "piece_tint_colours", py_tints);
        Py_DECREF(py_tints);

        return dict;
    }

    // 将 PlayerListEntry 转换为 Python 字典
    PyObject* convertToPythonObject(const PlayerListEntry& entry) {
        PyObject* dict = PyDict_New();
        if (dict == NULL) {
            PyErr_SetString(PyExc_MemoryError, "Failed to create Python dict for PlayerListEntry");
            return NULL;
        }

        // UUID
        PyObject* py_uuid = convertUUIDToPythonString(entry.UUID);
        PyDict_SetItemString(dict, "uuid", py_uuid);
        Py_DECREF(py_uuid);

        // EntityUniqueID
        PyObject* py_entity_id = PyLong_FromLongLong(entry.EntityUniqueID);
        PyDict_SetItemString(dict, "entity_unique_id", py_entity_id);
        Py_DECREF(py_entity_id);

        // EntityUniqueID*Py

        PyObject* py_entity_id_ = PyTextFromUtf8(std::to_string(~entry.EntityUniqueID >> 1).c_str());
        //PyObject* py_entity_id_ = PyLong_FromLongLong(~entry.EntityUniqueID >> 1);
        PyDict_SetItemString(dict, "entity_unique_id_netease", py_entity_id_);
        Py_DECREF(py_entity_id_);

        // Username
        PyObject* py_username = PyTextFromUtf8(entry.Username.c_str());
        PyDict_SetItemString(dict, "username", py_username);
        Py_DECREF(py_username);

        // XUID
        PyObject* py_xuid = PyTextFromUtf8(entry.XUID.c_str());
        PyDict_SetItemString(dict, "xuid", py_xuid);
        Py_DECREF(py_xuid);

        // PlatformChatID
        PyObject* py_platform_chat_id = PyTextFromUtf8(entry.PlatformChatID.c_str());
        PyDict_SetItemString(dict, "platform_chat_id", py_platform_chat_id);
        Py_DECREF(py_platform_chat_id);

        // BuildPlatform
        PyObject* py_build_platform = PyLong_FromLong(entry.BuildPlatform);
        PyDict_SetItemString(dict, "build_platform", py_build_platform);
        Py_DECREF(py_build_platform);

        // Skin
        PyObject* py_skin = convertToPythonObject(entry.Skin);
        PyDict_SetItemString(dict, "skin", py_skin);
        Py_DECREF(py_skin);

        // Teacher
        PyObject* py_teacher = PyBool_FromLong(entry.Teacher ? 1 : 0);
        PyDict_SetItemString(dict, "teacher", py_teacher);
        Py_DECREF(py_teacher);

        // Host
        PyObject* py_host = PyBool_FromLong(entry.Host ? 1 : 0);
        PyDict_SetItemString(dict, "host", py_host);
        Py_DECREF(py_host);

        // SubClient (网易特有)
        PyObject* py_sub_client = PyBool_FromLong(entry.SubClient ? 1 : 0);
        PyDict_SetItemString(dict, "sub_client", py_sub_client);
        Py_DECREF(py_sub_client);

        // Unknown (网易特有)
        PyObject* py_unknown = PyLong_FromLong(entry.Unknown);
        PyDict_SetItemString(dict, "unknown", py_unknown);
        Py_DECREF(py_unknown);

        return dict;
    }

    // 将 std::vector<PlayerListEntry> 转换为 Python 列表
    PyObject* convertToPythonObject(const std::vector<PlayerListEntry>& entries) {
        PyObject* py_list = PyList_New(entries.size());
        if (py_list == NULL) {
            PyErr_SetString(PyExc_MemoryError, "Failed to create Python list for PlayerListEntries");
            return NULL;
        }

        for (size_t i = 0; i < entries.size(); i++) {
            PyObject* py_entry = convertToPythonObject(entries[i]);
            PyList_SetItem(py_list, i, py_entry);
        }

        return py_list;
    }

    // 将 PlayerList 转换为 Python 字典
    PyObject* convertToPythonObject(const PlayerList& playerList) {
        PyObject* dict = PyDict_New();
        if (dict == NULL) {
            PyErr_SetString(PyExc_MemoryError, "Failed to create Python dict for PlayerList");
            return NULL;
        }

        // ActionType
        PyObject* py_action_type = PyLong_FromLong(playerList.ActionType);
        PyDict_SetItemString(dict, "action_type", py_action_type);
        Py_DECREF(py_action_type);

        // ActionType 字符串表示
        const char* action_str = (playerList.ActionType == PlayerListActionAdd) ? "add" : "remove";
        PyObject* py_action_str = PyTextFromUtf8(action_str);
        PyDict_SetItemString(dict, "action", py_action_str);
        Py_DECREF(py_action_str);

        // Entries
        PyObject* py_entries = convertToPythonObject(playerList.Entries);
        PyDict_SetItemString(dict, "entries", py_entries);
        Py_DECREF(py_entries);

        // 条目数量
        PyObject* py_count = PyLong_FromLong(static_cast<long>(playerList.Entries.size()));
        PyDict_SetItemString(dict, "count", py_count);
        Py_DECREF(py_count);

        return dict;
    }

    // ===== Respawn 相关类型转换 =====

    // 将 Vec3 转换为 Python 字典
    PyObject* convertVec3ToPythonObject(const Vec3& vec) {
        PyObject* dict = PyDict_New();
        if (dict == NULL) {
            PyErr_SetString(PyExc_MemoryError, "Failed to create Python dict for Vec3");
            return NULL;
        }

        PyObject* py_x = PyFloat_FromDouble(static_cast<double>(vec.x));
        PyDict_SetItemString(dict, "x", py_x);
        Py_DECREF(py_x);

        PyObject* py_y = PyFloat_FromDouble(static_cast<double>(vec.y));
        PyDict_SetItemString(dict, "y", py_y);
        Py_DECREF(py_y);

        PyObject* py_z = PyFloat_FromDouble(static_cast<double>(vec.z));
        PyDict_SetItemString(dict, "z", py_z);
        Py_DECREF(py_z);

        return dict;
    }

    // 将 Respawn 转换为 Python 字典
    PyObject* convertToPythonObject(const Respawn& respawn) {
        PyObject* dict = PyDict_New();
        if (dict == NULL) {
            PyErr_SetString(PyExc_MemoryError, "Failed to create Python dict for Respawn");
            return NULL;
        }

        // Position (Vec3)
        PyObject* py_position = convertVec3ToPythonObject(respawn.Position);
        PyDict_SetItemString(dict, "position", py_position);
        Py_DECREF(py_position);

        // State
        PyObject* py_state = PyLong_FromLong(respawn.State);
        PyDict_SetItemString(dict, "state", py_state);
        Py_DECREF(py_state);

        // State 字符串表示
        PyObject* py_state_str = PyTextFromUtf8(respawn.GetStateString());
        PyDict_SetItemString(dict, "state_string", py_state_str);
        Py_DECREF(py_state_str);

        // EntityRuntimeID
        PyObject* py_entity_id = PyLong_FromUnsignedLongLong(respawn.EntityRuntimeID);
        PyDict_SetItemString(dict, "entity_runtime_id", py_entity_id);
        Py_DECREF(py_entity_id);

        // 便捷布尔字段
        PyObject* py_searching = PyBool_FromLong(respawn.IsSearchingForSpawn() ? 1 : 0);
        PyDict_SetItemString(dict, "is_searching_for_spawn", py_searching);
        Py_DECREF(py_searching);

        PyObject* py_ready = PyBool_FromLong(respawn.IsReadyToSpawn() ? 1 : 0);
        PyDict_SetItemString(dict, "is_ready_to_spawn", py_ready);
        Py_DECREF(py_ready);

        PyObject* py_client_ready = PyBool_FromLong(respawn.IsClientReadyToSpawn() ? 1 : 0);
        PyDict_SetItemString(dict, "is_client_ready_to_spawn", py_client_ready);
        Py_DECREF(py_client_ready);

        return dict;
    }

public:
    // ===== CommandOutput 专用触发函数 =====

    // 触发 CommandOutput 事件，将 CommandOutput 转换为 Python 字典
    void triggerCommandOutputEvent(const std::string& event_name, const CommandOutput& output) {
        Logger::getInstance().log(LOG_INFO, std::string() + "[Engine] CommandOutput Event: " + event_name);
        PyGILState_STATE gstate = PyGILState_Ensure();

        PyObject* py_output = convertToPythonObject(output);
        PyObject* args_list = PyList_New(1);
        PyList_SetItem(args_list, 0, py_output);

        trigger_event(event_name.c_str(), args_list);
        Py_DECREF(args_list);

        PyGILState_Release(gstate);
    }

    // 触发 CommandOutput 事件，附带额外参数
    template<typename... Args>
    void triggerCommandOutputEvent(const std::string& event_name, const CommandOutput& output, Args&&... extra_args) {
        Logger::getInstance().log(LOG_INFO, std::string() + "[Engine] CommandOutput Event: " + event_name);
        PyGILState_STATE gstate = PyGILState_Ensure();

        // 创建参数列表: [output_dict, extra_args...]
        constexpr size_t extra_count = sizeof...(Args);
        PyObject* args_list = PyList_New(1 + extra_count);

        // 第一个参数是 CommandOutput
        PyObject* py_output = convertToPythonObject(output);
        PyList_SetItem(args_list, 0, py_output);

        // 添加额外参数
        if constexpr (extra_count > 0) {
            addArgumentsToList(args_list, 1, std::forward<Args>(extra_args)...);
        }

        trigger_event(event_name.c_str(), args_list);
        Py_DECREF(args_list);

        PyGILState_Release(gstate);
    }

    // ===== PlayerList 专用触发函数 =====

    // 触发 PlayerList 事件，将 PlayerList 转换为 Python 字典
    void triggerPlayerListEvent(const std::string& event_name, const PlayerList& playerList) {
        Logger::getInstance().log(LOG_INFO, std::string() + "[Engine] PlayerList Event: " + event_name);
        PyGILState_STATE gstate = PyGILState_Ensure();

        PyObject* py_player_list = convertToPythonObject(playerList);
        PyObject* args_list = PyList_New(1);
        PyList_SetItem(args_list, 0, py_player_list);

        trigger_event(event_name.c_str(), args_list);
        Py_DECREF(args_list);

        PyGILState_Release(gstate);
    }

    // 触发 PlayerList 事件，附带额外参数
    template<typename... Args>
    void triggerPlayerListEvent(const std::string& event_name, const PlayerList& playerList, Args&&... extra_args) {
        Logger::getInstance().log(LOG_INFO, std::string() + "[Engine] PlayerList Event: " + event_name);
        PyGILState_STATE gstate = PyGILState_Ensure();

        constexpr size_t extra_count = sizeof...(Args);
        PyObject* args_list = PyList_New(1 + extra_count);

        PyObject* py_player_list = convertToPythonObject(playerList);
        PyList_SetItem(args_list, 0, py_player_list);

        if constexpr (extra_count > 0) {
            addArgumentsToList(args_list, 1, std::forward<Args>(extra_args)...);
        }

        trigger_event(event_name.c_str(), args_list);
        Py_DECREF(args_list);

        PyGILState_Release(gstate);
    }

    // 触发单个 PlayerListEntry 事件
    void triggerPlayerListEntryEvent(const std::string& event_name, const PlayerListEntry& entry, bool isAdd) {
        Logger::getInstance().log(LOG_INFO, std::string() + "[Engine] PlayerListEntry Event: " + event_name);
        PyGILState_STATE gstate = PyGILState_Ensure();

        PyObject* args_list = PyList_New(2);

        PyObject* py_entry = convertToPythonObject(entry);
        PyList_SetItem(args_list, 0, py_entry);

        PyObject* py_is_add = PyBool_FromLong(isAdd ? 1 : 0);
        PyList_SetItem(args_list, 1, py_is_add);

        trigger_event(event_name.c_str(), args_list);
        Py_DECREF(args_list);

        PyGILState_Release(gstate);
    }

    // ===== Respawn 专用触发函数 =====

    // 触发 Respawn 事件，将 Respawn 转换为 Python 字典
    void triggerRespawnEvent(const std::string& event_name, const Respawn& respawn) {
        Logger::getInstance().log(LOG_INFO, std::string() + "[Engine] Respawn Event: " + event_name);
        PyGILState_STATE gstate = PyGILState_Ensure();

        PyObject* py_respawn = convertToPythonObject(respawn);
        PyObject* args_list = PyList_New(1);
        PyList_SetItem(args_list, 0, py_respawn);

        trigger_event(event_name.c_str(), args_list);
        Py_DECREF(args_list);

        PyGILState_Release(gstate);
    }

    // 触发 Respawn 事件，附带额外参数
    template<typename... Args>
    void triggerRespawnEvent(const std::string& event_name, const Respawn& respawn, Args&&... extra_args) {
        Logger::getInstance().log(LOG_INFO, std::string() + "[Engine] Respawn Event: " + event_name);
        PyGILState_STATE gstate = PyGILState_Ensure();

        constexpr size_t extra_count = sizeof...(Args);
        PyObject* args_list = PyList_New(1 + extra_count);

        PyObject* py_respawn = convertToPythonObject(respawn);
        PyList_SetItem(args_list, 0, py_respawn);

        if constexpr (extra_count > 0) {
            addArgumentsToList(args_list, 1, std::forward<Args>(extra_args)...);
        }

        trigger_event(event_name.c_str(), args_list);
        Py_DECREF(args_list);

        PyGILState_Release(gstate);
    }
};

// 全局便捷函数模板
template<typename... Args>
void triggerPythonEvent(const std::string& event_name, Args&&... args) {
    PythonEventEngine engine;
    engine.trigger(event_name, std::forward<Args>(args)...);
}

// CommandOutput 专用全局便捷函数
inline void triggerCommandOutputPythonEvent(const std::string& event_name, const CommandOutput& output) {
    PythonEventEngine engine;
    engine.triggerCommandOutputEvent(event_name, output);
}

// CommandOutput 专用全局便捷函数 (带额外参数)
template<typename... Args>
void triggerCommandOutputPythonEvent(const std::string& event_name, const CommandOutput& output, Args&&... extra_args) {
    PythonEventEngine engine;
    engine.triggerCommandOutputEvent(event_name, output, std::forward<Args>(extra_args)...);
}

// ===== PlayerList 专用全局便捷函数 =====

// PlayerList 专用全局便捷函数
inline void triggerPlayerListPythonEvent(const std::string& event_name, const PlayerList& playerList) {
    PythonEventEngine engine;
    engine.triggerPlayerListEvent(event_name, playerList);
}

// PlayerList 专用全局便捷函数 (带额外参数)
template<typename... Args>
void triggerPlayerListPythonEvent(const std::string& event_name, const PlayerList& playerList, Args&&... extra_args) {
    PythonEventEngine engine;
    engine.triggerPlayerListEvent(event_name, playerList, std::forward<Args>(extra_args)...);
}

// PlayerListEntry 专用全局便捷函数
inline void triggerPlayerListEntryPythonEvent(const std::string& event_name, const PlayerListEntry& entry, bool isAdd) {
    PythonEventEngine engine;
    engine.triggerPlayerListEntryEvent(event_name, entry, isAdd);
}

// ===== Respawn 专用全局便捷函数 =====

// Respawn 专用全局便捷函数
inline void triggerRespawnPythonEvent(const std::string& event_name, const Respawn& respawn) {
    PythonEventEngine engine;
    engine.triggerRespawnEvent(event_name, respawn);
}

// Respawn 专用全局便捷函数 (带额外参数)
template<typename... Args>
void triggerRespawnPythonEvent(const std::string& event_name, const Respawn& respawn, Args&&... extra_args) {
    PythonEventEngine engine;
    engine.triggerRespawnEvent(event_name, respawn, std::forward<Args>(extra_args)...);
}

